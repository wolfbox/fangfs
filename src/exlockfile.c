#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sched.h>
#include "exlockfile.h"
#include "error.h"

bool exlock_try_obtain(const char* path) {
	// Reset errno so people can get an error code if they like.
	errno = 0;

	int fd = open(path, O_WRONLY|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR);
	if(fd < 0) {
		return false;
	} else {
		// Write in our PID, just to be a good system citizen
		char buf[12];
		pid_t pid = getpid();
		snprintf(buf, sizeof(buf), "%d", pid);
		ssize_t pid_len = strlen(buf);

		// It's tough beans if we can't write. This shouldn't ever happen, and
		// if it does, our best shot is to just ignore it.
		if(write(fd, buf, pid_len) < pid_len) {}
		close(fd);
		return true;
	}
}

int exlock_obtain(const char* path) {
	while(1) {
		bool have_lock = exlock_try_obtain(path);
		if(have_lock) {
			return 0;
		} else {
			if(errno != EEXIST) {
				return STATUS_CHECK_ERRNO;
			}
		}

		// Avoid starving out other threads while we try to get this lock.
		// XXX Should we sleep here instead?
		sched_yield();
	}
}

int exlock_release(const char* path) {
	if(unlink(path) < 0) {
		return STATUS_CHECK_ERRNO;
	}

	return 0;
}
