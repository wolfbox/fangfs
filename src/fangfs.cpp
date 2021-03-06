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
#include "BufferEncryption.h"
#include "file.h"
#include "error.h"
#include "compat/compat.h"

// Returns 0 on success, 1 if the directory is populated, and -1 on error.
static int initialize_empty_filesystem(FangFS& self) {
	// Make sure the source is a directory, and check if it's empty.
	// Emptiness is checked to avoid creating a new source filesystem in a
	// populated directory.
	{
		DIR* dir = opendir(self.source);
		if(dir == nullptr) {
			return STATUS_CHECK_ERRNO;
		}

		int n_entries = 0;
		struct dirent entry;
		struct dirent* result = nullptr;
		errno = 0;
		while(readdir_r(dir, &entry, &result) == 0) {
			if(result == nullptr) {
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
	randombytes_buf(self.master_key, sizeof(self.master_key));

	// Create our metafile
	metafile_write(self.metafile);

	return 0;
}

int fangfs_mknod(FangFS& self, const char* path, mode_t m, dev_t d) {
	Buffer real_path;
	path_resolve(self, path, real_path);

	{
		int status = mknod(reinterpret_cast<char*>(real_path.buf), m, d);
		if(status < 0) {
			return -errno;
		}
	}

	return 0;
}

int fangfs_truncate(FangFS& self, const char* path, off_t end) {
	Buffer real_path;
	path_resolve(self, path, real_path);

	int fd = open(reinterpret_cast<char*>(real_path.buf), O_RDWR, 0);
	if(fd < 0) { return errno; }

	struct fuse_file_info fi;
	fi.fh = fd;
	fangfs_ftruncate(self, path, end, &fi);

	if(close(fd) < 0) { return errno; }
	return 0;
}

int fangfs_ftruncate(FangFS& self, const char* path, off_t end, struct fuse_file_info* fi) {
	if(fi->fh == 0) {
		return -EINVAL;
	}

	fprintf(stderr, "Truncating %s to %lu\n", path, end);
	if(end == 0) {
		// This is common, so let's fast-path this
		return ftruncate(fi->fh, 0);
	}

	errno = ENOSYS;
	return -errno;
}

int fangfs_unlink(FangFS& self, const char* path) {
	Buffer real_path;
	path_resolve(self, path, real_path);

	if(unlink(reinterpret_cast<char*>(real_path.buf)) != 0) {
		return -errno;
	}

	return 0;
}

int fangfs_fsinit(FangFS& self, const char* source) {
	self.source = source;

	// Prevent swapping out the master key
	{
		int error = sodium_mlock(self.master_key, sizeof(self.master_key));
		if(error) { return STATUS_ERROR; }
	}

	// If we already have a metafile, parse it.  Otherwise, initialize it.
	int status = metafile_init(self.metafile, source);
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

void fangfs_fsclose(FangFS& self) {
	metafile_free(self.metafile);

	// Zeros the key and allows its page to be swapped again.
	sodium_munlock(self.master_key, sizeof(self.master_key));
}

int fangfs_getattr(FangFS& self, const char* path, struct stat* stbuf) {
	Buffer real_path;
	path_resolve(self, path, real_path);

	if(stat(reinterpret_cast<char*>(real_path.buf), stbuf) < 0) {
		return -errno;
	}

	return 0;
}

int fangfs_open(FangFS& self, const char* path, struct fuse_file_info* fi) {
	Buffer real_path;
	path_resolve(self, path, real_path);

	// Writing to a block always requires reading it in
	int flags = fi->flags;
	if(flags & O_WRONLY) {
		flags = O_RDWR;
	}

	int fd = open(reinterpret_cast<char*>(real_path.buf), flags);
	int new_errno = errno;

	if(fd < 0) {
		return new_errno;
	}

	printf("Open OK\n");
	fi->fh = fd;
	return 0;
}

int fangfs_read(FangFS& self, char* buf, size_t size, off_t offset, \
                struct fuse_file_info* fi) {
	if(fi->fh == 0) {
		return -EINVAL;
	}

	FangFile file(self, fi->fh);
	return fang_file_read(file, offset, size, reinterpret_cast<uint8_t*>(buf));
}

int fangfs_write(FangFS& self, const char* buf, size_t size, off_t offset, \
                 struct fuse_file_info* fi) {
	if(fi->fh == 0) {
		return -EINVAL;
	}

	FangFile file(self, fi->fh);
	return fang_file_write(file, offset, size, reinterpret_cast<const uint8_t*>(buf));
}

int fangfs_mkdir(FangFS& self, const char* path, mode_t mode) {
	Buffer realpath;
	path_resolve(self, path, realpath);

	if(mkdir(reinterpret_cast<char*>(realpath.buf), mode) < 0) {
		return -errno;
	}

	return 0;
}

int fangfs_opendir(FangFS& self, const char* path, struct fuse_file_info* fi) {
	Buffer real_path;
	path_resolve(self, path, real_path);

	DIR* dir = opendir(reinterpret_cast<char*>(real_path.buf));
	int new_errno = errno;

	if(dir == nullptr) {
		return new_errno;
	}

	fi->fh = dirfd(dir);
	return 0;
}

int fangfs_readdir(FangFS& self, const char* path, void* buf,
                        fuse_fill_dir_t filler, off_t offset,
                        struct fuse_file_info* fi) {
	DIR* dir = fdopendir(fi->fh);
	if(dir == nullptr) {
		return -errno;
	}

	Buffer decrypted;
	Buffer fullpath;

	struct dirent entry;
	struct dirent* result;
	while(readdir_r(dir, &entry, &result) == 0) {
		if(result == nullptr) {
			return 0;
		}

		// Skip over "special" names
		if(entry.d_name[0] == '_') { continue; }
		if(strcmp(entry.d_name, ".") == 0 ||
		   strcmp(entry.d_name, "..") == 0) {
			filler(buf, entry.d_name, nullptr, 0);
			continue;
		}

		// Decrypt the filename
		int status = path_decrypt(self, entry.d_name, decrypted);
		if(status < 0) {
			fprintf(stderr, "Tampering detected on file %s\n", entry.d_name);
			continue;
		}

		// Strip out the hash
		const char* filename = reinterpret_cast<char*>(decrypted.buf) +
		                       crypto_generichash_BYTES;

		// Verify the hash, preventing files from being moved around by someone
		// outside the encrypted filesystem.
		{
			path_join(path, filename, fullpath);
			uint8_t path_hash[crypto_generichash_BYTES];
			crypto_generichash(path_hash, sizeof(path_hash),
			                   reinterpret_cast<const uint8_t*>(fullpath.buf),
			                   fullpath.len, nullptr, 0);
			if(sodium_memcmp(path_hash, decrypted.buf, sizeof(path_hash)) != 0) {
				continue;
			}
		}

		filler(buf, filename, nullptr, 0);
	}

	// Something went haywire
	int new_errno = errno;

	return -new_errno;
}

int fangfs_close(FangFS& self, struct fuse_file_info* fi) {
	close(fi->fh);
	return 0;
}

void path_resolve(FangFS& self, const char* path, Buffer& outbuf) {
	if(strcmp(path, "/") == 0) {
		buf_load_string(outbuf, self.source);
		return;
	}

	Buffer path_buf;
	buf_load_string(path_buf, path);

	Buffer encrypted_path;
	Buffer tmpbuf;

	buf_load_string(outbuf, self.source);
	path_building_for_each(path_buf, [&](const Buffer& cur) {
		path_encrypt(self, reinterpret_cast<char*>(cur.buf), encrypted_path);
		buf_copy(outbuf, tmpbuf);
		path_join(reinterpret_cast<char*>(tmpbuf.buf),
		          reinterpret_cast<char*>(encrypted_path.buf),
		          outbuf);
	});
}

void path_encrypt(FangFS& self, const char* orig, Buffer& outbuf) {
	const size_t orig_len = strlen(orig);

	// Four steps to this
	// 1) Compute the hash of the whole path
	uint8_t path_hash[crypto_generichash_BYTES];
	crypto_generichash(path_hash, sizeof(path_hash), reinterpret_cast<const uint8_t*>(orig),
	                   orig_len, nullptr, 0);

	// 2) Isolate just the filename
	const char* basename = path_get_basename(orig);

	// 3) Encrypt the concatenation of the hash with the filename
	Buffer inbuf;
	buf_grow(inbuf, sizeof(path_hash) + strlen(basename) + 1);
	memcpy(inbuf.buf, path_hash, sizeof(path_hash));
	strcpy(reinterpret_cast<char*>(inbuf.buf) + sizeof(path_hash), basename);
	inbuf.len = inbuf.buf_len;

	Buffer ciphertext;
	buf_encrypt(inbuf, self.metafile.filename_nonce, self.master_key, ciphertext);

	// 4) Encode
	base32_enc(ciphertext, outbuf);
	fprintf(stderr, "Encrypted: %s\n", outbuf.buf);
}

int path_decrypt(FangFS& self, const char* orig, Buffer& outbuf) {
	Buffer ciphertext;

	base32_dec(orig, ciphertext);

	int result = buf_decrypt(ciphertext, self.metafile.filename_nonce, self.master_key, outbuf);

	if(result != 0) {
		return STATUS_TAMPERING;
	}

	return 0;
}
