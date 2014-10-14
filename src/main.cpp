#include "fangfs.h"

#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "error.h"

static fangfs_t fangfs;

static int fangfs_fuse_mknod(const char* path, mode_t m, dev_t d) {
	return fangfs_mknod(&fangfs, path, m, d);
}

static int fangfs_fuse_open(const char* path, struct fuse_file_info* fi) {
	return fangfs_open(&fangfs, path, fi);
}

static int fangfs_fuse_release(const char* path, struct fuse_file_info* fi) {
	return fangfs_close(&fangfs, fi);
}

static int fangfs_fuse_getattr(const char* path, struct stat* stbuf) {
	return fangfs_getattr(&fangfs, path, stbuf);
}

static int fangfs_fuse_read(const char* path, char* buf, size_t size, \
                            off_t offset, struct fuse_file_info* fi) {
	return fangfs_read(&fangfs, buf, size, offset, fi);
}

static int fangfs_fuse_opendir(const char* path, struct fuse_file_info* fi) {
	return fangfs_opendir(&fangfs, path, fi);
}

static int fangfs_fuse_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                               off_t offset, struct fuse_file_info* fi) {
	return fangfs_readdir(&fangfs, path, buf, filler, offset, fi);
}

static int fangfs_fuse_releasedir(const char* path, struct fuse_file_info* fi) {
	DIR* dir = fdopendir(fi->fh);
	if(dir == NULL) { return -errno; }

	if(closedir(fdopendir(fi->fh)) < 0) {
		return -errno;
	}

	return 0;
}

static struct fuse_operations fang_ops;

void handle_signal(int signum) {
	fangfs_fsclose(&fangfs);

	if(signum == SIGINT ||
	   signum == SIGTERM) {
	   exit(0);
	}
}

int main(int argc, char** argv) {
	fang_ops.mknod = fangfs_fuse_mknod;
    fang_ops.open = fangfs_fuse_open;
    fang_ops.release = fangfs_fuse_release;
    fang_ops.getattr = fangfs_fuse_getattr;
    fang_ops.read = fangfs_fuse_read;
    fang_ops.opendir = fangfs_fuse_opendir;
    fang_ops.readdir = fangfs_fuse_readdir;
    fang_ops.releasedir = fangfs_fuse_releasedir;

	if(argc < 3) {
		return 1;
	}

	const char* source_dir = argv[1];
	argc--;
	argv++;

	{
		const int status = fangfs_fsinit(&fangfs, source_dir);
		if(status < 0) {
			fprintf(stderr, "Initialization error: %d.\n", status);
			if(status == STATUS_CHECK_ERRNO) {
				fprintf(stderr, "%s\n", strerror(errno));
			}
			return 1;
		}
	}

	// If we recieve a shutdown signal, we still want to clear any secret
	// memory.
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = handle_signal;
	sigaction(SIGINT, &action, NULL);
	sigaction(SIGTERM, &action, NULL);

	const int status = fuse_main(argc, argv, &fang_ops, NULL);

	fangfs_fsclose(&fangfs);
	return status;
}