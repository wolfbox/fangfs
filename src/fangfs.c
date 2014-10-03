#include "fangfs.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <attr/xattr.h>
#include "util.h"

int fangfs_fsinit(fangfs_t* self, const char* source) {
	int error = sodium_mlock(self->key, sizeof(self->key));
	if(error) { return 1; }

	// Check for errors in the source
	{
		struct stat st_buf;
		if(stat(source, &st_buf) != 0) {
			return errno;
		}

		if(!S_ISDIR(st_buf.st_mode)) {
			return ENOTDIR;;
		}
	}

	// Figure out our block size
	{
		struct statvfs st_buf;
		if(statvfs(source, &st_buf) != 0) {
			return errno;
		}
		self->blocksize = st_buf.f_bsize;
	}

	self->source = source;

	return 0;
}

int fangfs_getattr(fangfs_t* self, const char* path, struct stat* stbuf) {
	memset(stbuf, 0, sizeof(struct stat));

	if(strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if(strcmp(path, "/.viminfo") == 0) {
		stbuf->st_mode = S_IFREG | 0644;
		stbuf->st_nlink = 1;
		stbuf->st_size = 100;
	} else {
		return -ENOENT;
	}

	return 0;
}

int fangfs_open(fangfs_t* self, const char* path, struct fuse_file_info* fi) {
    char* real_path = malloc(1000);
    int status = path_join(self->source, path, real_path, 1000);
    if(status != 0) {
    	free(real_path);
    	return 1;
    }

	int fd = open(real_path, fi->flags);
	free(real_path);
	if(fd == 0) {
		return errno;
	}

	fi->fh = fd;
	return 0;
}

int fangfs_read(fangfs_t* self, char* buf, size_t size, off_t offset, \
                struct fuse_file_info* fi) {
	if(fi->fh == 0) {
		return EINVAL;
	}

	lseek(fi->fh, offset, SEEK_SET);
	ssize_t n_read = read(fi->fh, buf, size);

	return n_read;
}

int fangfs_close(fangfs_t* self, struct fuse_file_info* fi) {
	close(fi->fh);
	return 0;
}

void fangfs_fsclose(fangfs_t* self) {
	sodium_memzero(self->key, sizeof(self->key));
	sodium_munlock(self->key, sizeof(self->key));
}
