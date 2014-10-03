#include "fangfs.h"

#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

static fangfs_t fangfs;

static int fangfs_fuse_open(const char* path, struct fuse_file_info* fi) {
	return fangfs_open(&fangfs, path, fi);
}

static int fangfs_fuse_close(const char* path, struct fuse_file_info* fi) {
	return fangfs_close(&fangfs, fi);
}

static int fangfs_fuse_getattr(const char* path, struct stat* stbuf) {
	return fangfs_getattr(&fangfs, path, stbuf);
}

static int fangfs_fuse_read(const char* path, char* buf, size_t size, \
                            off_t offset, struct fuse_file_info* fi) {
	return fangfs_read(&fangfs, buf, size, offset, fi);
}

static int fangfs_fuse_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                               off_t offset, struct fuse_file_info* fi) {
	if(strcmp(path, "/") != 0) {
		return -ENOENT;
	}

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, ".viminfo", NULL, 0);
	return 0;
}

static struct fuse_operations fang_ops = {
	.open = fangfs_fuse_open,
    .release = fangfs_fuse_close,
    .getattr = fangfs_fuse_getattr,
    .read = fangfs_fuse_read,
    .readdir = fangfs_fuse_readdir,
};

void handle_signal(int signum) {
	fangfs_fsclose(&fangfs);

	if(signum == SIGINT ||
	   signum == SIGTERM) {
	   exit(0);
	}
}

int main(int argc, char** argv) {
	if(argc < 3) {
		return 1;
	}

	const char* source_dir = argv[1];
	argc--;
	argv++;

	{
		const int status = fangfs_fsinit(&fangfs, source_dir);
		if(status != 0) {
			fprintf(stderr, "Initialization error: %s.\n", strerror(status));
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
