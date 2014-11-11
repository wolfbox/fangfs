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
#include "util.h"
#include "error.h"
#include "compat/compat.h"

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

int fangfs_mknod(fangfs_t* self, const char* path, mode_t m, dev_t d) {
	Buffer real_path;

	{
		int status = path_resolve(self, path, real_path);
		if(status != 0) {
			return -status;
		}
	}

	{
		int status = mknod((char*)real_path.buf, m, d);
		if(status < 0) {
			return -errno;
		}
	}

	printf("mknod OK\n");
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
		return status;
	}

	return 0;
}

void fangfs_fsclose(fangfs_t* self) {
	metafile_free(&self->metafile);

	// Zeros the key and allows its page to be swapped again.
	sodium_munlock(self->master_key, sizeof(self->master_key));
}

int fangfs_getattr(fangfs_t* self, const char* path, struct stat* stbuf) {
	Buffer real_path;

	int status = path_resolve(self, path, real_path);
    if(status != 0) {
    	return -ENOMEM;
    }

	if(stat((char*)real_path.buf, stbuf) < 0) {
		return -errno;
	}

	return 0;
}

int fangfs_open(fangfs_t* self, const char* path, struct fuse_file_info* fi) {
	Buffer real_path;
	int status = path_resolve(self, path, real_path);

	if(status != 0) {
		return status;
	}

	int fd = open((char*)real_path.buf, fi->flags);
	int new_errno = errno;

	if(fd < 0) {
		return new_errno;
	}

	printf("Open OK\n");
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

int fangfs_opendir(fangfs_t* self, const char* path, struct fuse_file_info* fi) {
	Buffer real_path;

	int status = path_resolve(self, path, real_path);
	if(status != 0) {
		return status;
	}

	DIR* dir = opendir((char*)real_path.buf);
	int new_errno = errno;

	if(dir == NULL) {
		return new_errno;
	}

	fi->fh = dirfd(dir);
	return 0;
}

int fangfs_readdir(fangfs_t* self, const char* path, void* buf,
                        fuse_fill_dir_t filler, off_t offset,
                        struct fuse_file_info* fi) {
	DIR* dir = fdopendir(fi->fh);
	if(dir == NULL) {
		return -errno;
	}

	Buffer decrypted;

	struct dirent entry;
	struct dirent* result;
	while(readdir_r(dir, &entry, &result) == 0) {
		if(result == NULL) {
			return 0;
		}

		// Skip over "special" names
		if(entry.d_name[0] == '_') { continue; }
		if(strcmp(entry.d_name, ".") == 0 ||
		   strcmp(entry.d_name, "..") == 0) {
			filler(buf, entry.d_name, NULL, 0);
			continue;
		}

		// Decrypt the filename
		int status = path_decrypt(self, entry.d_name, decrypted);
		if(status < 0) {
			fprintf(stderr, "Tampering detected on file %s\n", entry.d_name);
			continue;
		}
		filler(buf, (char*)decrypted.buf, NULL, 0);
	}

	// Something went haywire
	int new_errno = errno;

	return -new_errno;
}

int fangfs_close(fangfs_t* self, struct fuse_file_info* fi) {
	close(fi->fh);
	return 0;
}

int path_resolve(fangfs_t* self, const char* path, Buffer& outbuf) {
	if(strcmp(path, "/") == 0) {
		buf_load_string(outbuf, self->source);
		return 0;
	}

	Buffer encrypted_path;
	path_encrypt(self, path, encrypted_path);

	int status = path_join(self->source, (char*)encrypted_path.buf, outbuf);

	if(status != 0) {
		outbuf.len = 0;
		return ENOMEM;
	}
	fprintf(stderr, "%s\n", outbuf.buf);

	return 0;
}

int path_encrypt(fangfs_t* self, const char* orig, Buffer& outbuf) {
	const size_t orig_len = strlen(orig);
	Buffer ciphertext;
	buf_grow(ciphertext, orig_len + crypto_secretbox_MACBYTES);
	crypto_secretbox_easy(ciphertext.buf, (uint8_t*)orig, orig_len,
	                      self->metafile.filename_nonce, self->master_key);
	ciphertext.len = orig_len + crypto_secretbox_MACBYTES;

	base32_enc(ciphertext, outbuf);

	return 0;
}

int path_decrypt(fangfs_t* self, const char* orig, Buffer& outbuf) {
	Buffer ciphertext;

	base32_dec(orig, ciphertext);
	buf_grow(outbuf, ciphertext.len - crypto_secretbox_MACBYTES + 1);

	int result = crypto_secretbox_open_easy(outbuf.buf, ciphertext.buf,
	                                        ciphertext.len,
	                                        self->metafile.filename_nonce,
	                                        self->master_key);
	if(result < 0) {
		outbuf.len = 0;
		return STATUS_TAMPERING;
	}

	outbuf.len = ciphertext.len - crypto_secretbox_MACBYTES;
	outbuf.buf[outbuf.len] = '\0';

	return 0;
}
