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

static int initialize_empty_filesystem(fangfs_t* self) {
	// Create our master key
	randombytes_buf(self->master_key, sizeof(self->master_key));

	// Create our metafile
	metafile_write(&self->metafile);

	return 0;
}

static int load_metafile(fangfs_t* self, bool is_empty) {
	bool meta_exists = true;
	struct stat st_buf;
	if(stat(self->metafile.metapath, &st_buf) != 0) {
		if(errno != ENOENT) {
			return errno;
		}

		meta_exists = false;
	}

	if(meta_exists) {
		// The metafile already exists: parse it.
		int status = metafile_parse(&self->metafile);
		if(status != 0) { return status; }
		return 0;
	} else {
		// The metafile is missing: create it.
		// Let's not trample over an existing populated directory, hmm?
		if(!is_empty) {
			return ENOTEMPTY;
		}

		initialize_empty_filesystem(self);
	}

	return 0;
}

int fangfs_fsinit(fangfs_t* self, const char* source) {
	bool is_empty = false;
	self->source = source;

	// Prevent swapping out the master key
	{
		int error = sodium_mlock(self->master_key, sizeof(self->master_key));
		if(error) { return 1; }
	}

	// Make sure the source is a directory, and check if it's empty.
	// Emptiness is checked to avoid creating a new source filesystem in a
	// populated directory.
	{
		DIR* dir = opendir(source);
		if(dir == NULL) {
			return errno;
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

		if(errno != 0) { return errno; }
		is_empty = (n_entries <= 2);
	}

	// Figure out our block size
	uint32_t block_size = 0;
	{
		struct statvfs st_buf;
		if(statvfs(source, &st_buf) != 0) {
			return errno;
		}
		block_size = st_buf.f_bsize;
	}

	// If we already have a metafile, parse it.  Otherwise, initialize it.
	int status = metafile_init(&self->metafile, block_size, source);
	if(status == 0) {
		status = load_metafile(self, is_empty);
	}

	return status;
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
    if(status != 0) {
    	buf_free(&real_path);
    	return status;
    }

	if(stat((char*)real_path.buf, stbuf) < 0) {
		return errno;
	}

	return 0;
}

int fangfs_open(fangfs_t* self, const char* path, struct fuse_file_info* fi) {
	buf_t real_path;
	buf_init(&real_path);
    int status = path_join(self->source, path, &real_path);
    if(status != 0) {
    	buf_free(&real_path);
    	return 1;
    }

	int fd = open((char*)real_path.buf, fi->flags);
	buf_free(&real_path);

	if(fd < 0) {
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
