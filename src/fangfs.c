#include "fangfs.h"

#include <dirent.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <attr/xattr.h>
#include "util.h"
#include "error.h"

// Returns 0 on success, 1 if the directory is populated, and -1 on error.
static int initialize_empty_filesystem(fangfs_t* self) {
	// Make sure the source is a directory, and check if it's empty.
	// Emptiness is checked to avoid creating a new source filesystem in a
	// populated directory.
	{
		DIR* dir = opendir(self->source);
		if(dir == NULL) {
			return STATUS_CHECK_ERRNO;
		}

		int n_entries = 0;
		struct dirent entry;
		struct dirent* result = NULL;
		errno = 0;
		while(readdir_r(dir, &entry, &result) == 0) {
			if(result == NULL) {
				break;
			}
			n_entries += 1;
		}
		closedir(dir);

		if(errno != 0) { return STATUS_CHECK_ERRNO; }

		// At this point, we should have the metafile and the lockfile.
		if(n_entries > 4) { return 1; }
	}

	// Create our master key
	randombytes_buf(self->master_key, sizeof(self->master_key));

	// Create our metafile
	metafile_write(&self->metafile);

	return 0;
}

int fangfs_fsinit(fangfs_t* self, const char* source) {
	self->source = source;

	// Prevent swapping out the master key
	{
		int error = sodium_mlock(self->master_key, sizeof(self->master_key));
		if(error) { return STATUS_ERROR; }
	}

	// If we already have a metafile, parse it.  Otherwise, initialize it.
	int status = metafile_init(&self->metafile, source);
	if(status == 0) {
		int initstatus = initialize_empty_filesystem(self);
		int new_errno = 0;
		if(initstatus > 0) { new_errno = ENOTEMPTY; }
		if(initstatus != 0) {
			fangfs_fsclose(self);
			errno = new_errno;
			return STATUS_CHECK_ERRNO;
		}
	} else if(status < 0) {
		return STATUS_ERROR;
	}

	return 0;
}

void fangfs_fsclose(fangfs_t* self) {
	metafile_free(&self->metafile);

	// Zeros the key and allows its page to be swapped again.
	sodium_munlock(self->master_key, sizeof(self->master_key));
}

int fangfs_getattr(fangfs_t* self, const char* path, struct stat* stbuf) {
	buf_t real_path;
	buf_init(&real_path);
    int status = path_join(self->source, path, &real_path);
    if(status < 0) {
    	buf_free(&real_path);
    	return status;
    }

	if(stat((char*)real_path.buf, stbuf) < 0) {
		return STATUS_CHECK_ERRNO;
	}

	return 0;
}

int fangfs_open(fangfs_t* self, const char* path, struct fuse_file_info* fi) {
	buf_t real_path;
	buf_init(&real_path);
    int status = path_join(self->source, path, &real_path);
    if(status != 0) {
    	buf_free(&real_path);
    	return STATUS_ERROR;
    }

	int fd = open((char*)real_path.buf, fi->flags);
	int new_errno = errno;
	buf_free(&real_path);

	if(fd < 0) {
		errno = new_errno;
		return STATUS_CHECK_ERRNO;
	}

	fi->fh = fd;
	return 0;
}

int fangfs_read(fangfs_t* self, char* buf, size_t size, off_t offset, \
                struct fuse_file_info* fi) {
	if(fi->fh == 0) {
		return EINVAL;
	}

	{
		int result = lseek(fi->fh, offset, SEEK_SET);
		if(result < 0) { return errno; }
	}
	ssize_t n_read = read(fi->fh, buf, size);

	return n_read;
}

int fangfs_close(fangfs_t* self, struct fuse_file_info* fi) {
	close(fi->fh);
	return 0;
}
