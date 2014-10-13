#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include "util.h"
#include "exlockfile.h"
#include "metafile.h"
#include "error.h"

// Don't use more than 128M of memory in our kdf when using automatic settings.
#define MAX_MEM_LIMIT 1024 * 1024 * 1024

static int metafile_init_new(metafile_t* self) {
	// Figure out our block size
	{
		struct statvfs st_buf;
		if(statvfs(self->metapath, &st_buf) != 0) {
			return STATUS_ERROR;
		}
		self->block_size = st_buf.f_bsize;
	}

	// Generate a random filename nonce
	randombytes_buf(self->filename_nonce, sizeof(self->filename_nonce));

	return 0;
}

static metafield_t* metafile_append_key(metafile_t* self) {
	if(self->n_keys >= METAFILE_MAX_KEYS) {
		return NULL;
	}

	metafield_t* field = &self->keys[self->n_keys];
	self->n_keys += 1;

	memset(field, 0, sizeof(metafield_t));
	return field;
}

void metafield_parse(metafield_t* self, const uint8_t inbuf[META_FIELD_LEN]) {
	uint8_t const* cur = inbuf;

	self->opslimit = u32_from_le((uint32_t)(u32_from_bytes(cur)));
	cur += sizeof(uint32_t);

	self->memlimit = u32_from_le((uint32_t)(u32_from_bytes(cur)));
	cur += sizeof(uint32_t);

	memcpy(self->nonce, cur, sizeof(self->nonce));
	cur += sizeof(self->nonce);

	memcpy(self->encrypted_key, cur, sizeof(self->encrypted_key));
}

void metafield_serialize(metafield_t* self, uint8_t outbuf[META_FIELD_LEN]) {
	uint8_t* cur = outbuf;

	memcpy(cur, &self->opslimit, sizeof(self->opslimit));
	cur += sizeof(self->opslimit);

	memcpy(cur, &self->memlimit, sizeof(self->memlimit));
	cur += sizeof(self->memlimit);

	memcpy(cur, self->nonce, sizeof(self->nonce));
	cur += sizeof(self->nonce);

	memcpy(cur, self->encrypted_key, sizeof(self->encrypted_key));
}

static int get_paths(metafile_t* self, const char* sourcepath) {
	// Create the path to the metafile
	Buffer path_buf;
	if(path_join(sourcepath, METAFILE_NAME, path_buf) != 0) {
		return STATUS_ERROR;
	}
	self->metapath = buf_copy_string(path_buf);
	if(self->metapath == NULL) {
		return STATUS_ERROR;
	}

	// Create the path to the lockfile
	if(path_join(sourcepath, METAFILE_LOCK, path_buf) != 0) {
		return STATUS_ERROR;
	}

	self->lockpath = buf_copy_string(path_buf);
	if(self->lockpath == NULL) {
		return STATUS_ERROR;
	}

	return 0;
}

int metafile_init(metafile_t* self, const char* sourcepath) {
	self->version = FANGFS_META_VERSION;
	memset(self->filename_nonce, 0, sizeof(self->filename_nonce));
	self->n_keys = 0;
	memset(self->keys, 0, sizeof(self->keys));

	{
		int status = get_paths(self, sourcepath);
		if(status < 0) { return status; }
	}

	{
		// Obtain our lock file so we can safely open up the metafile
		int status = exlock_obtain(self->lockpath);
		if(status != 0) { return status; }
	}

	// Open the metafile
	self->metafd = open(self->metapath, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
	if(self->metafd < 0) {
		return STATUS_CHECK_ERRNO;
	}

	// Figure out if the metafile is empty or not
	{
		struct stat info;
		if(fstat(self->metafd, &info) < 0) {
			close(self->metafd);
			return STATUS_ERROR;
		}

		if(info.st_size > 0) {
			// The metafile exists.
			int status = metafile_parse(self);
			if(status < 0) { return status; }
			return 1;
		}

		// The metafile does not yet exist. Initialize.
		if(metafile_init_new(self) < 0) {
			close(self->metafd);
			return STATUS_ERROR;
		}
	}

	return 0;
}

int metafile_parse(metafile_t* self) {
	printf("Parsing\n");

	if(lseek(self->metafd, 0, SEEK_SET) < 0) {
		return STATUS_CHECK_ERRNO;
	}

	// Read the constant headers
	{
		ssize_t n_read = read(self->metafd, &self->version, sizeof(self->version));
		if(n_read != 1) {
			return STATUS_CHECK_ERRNO;
		}

		n_read = read(self->metafd, &self->block_size, sizeof(self->block_size));
		if(n_read < (ssize_t)sizeof(self->block_size)) {
			return STATUS_CHECK_ERRNO;
		}
		self->block_size = u32_from_le(self->block_size);

		n_read = read(self->metafd, &self->filename_nonce, sizeof(self->filename_nonce));
		if(n_read < (ssize_t)sizeof(self->filename_nonce)) {
			return STATUS_CHECK_ERRNO;
		}
	}

	// Read records until EOF
	while(1) {
		uint8_t buf[META_FIELD_LEN];
		ssize_t n_read = 0;

		// Read the current record until either EOF or it's finished.
		while(n_read < (ssize_t)META_FIELD_LEN) {
			ssize_t n = read(self->metafd, buf, (META_FIELD_LEN-n_read));
			if(n == 0) {
				// EOF
				return 0;
			}
			n_read += n;
		}

		metafield_t* field = metafile_append_key(self);
		metafield_parse(field, buf);
	}
}

int metafile_write(metafile_t* self) {
	// XXX We should back up the metafile BEFORE overwriting it.
	// Just in case.
	if(lseek(self->metafd, 0, SEEK_SET) < 0) {
		return STATUS_CHECK_ERRNO;
	}

	size_t outbuf_len = sizeof(self->version) +
	                    sizeof(self->block_size) +
	                    sizeof(self->filename_nonce) +
	                    (META_FIELD_LEN * self->n_keys);
	uint8_t* outbuf = (uint8_t*)malloc(outbuf_len);
	uint8_t* cur = outbuf;

	memcpy(cur, &self->version, sizeof(self->version));
	cur += sizeof(self->version);

	uint32_t block_size = u32_to_le(self->block_size);
	memcpy(cur, &block_size, sizeof(block_size));
	cur += sizeof(block_size);

	memcpy(cur, self->filename_nonce, sizeof(self->filename_nonce));
	cur += sizeof(self->filename_nonce);

	for(size_t i = 0; i < self->n_keys; i += 1) {
		metafield_serialize(&self->keys[i], cur);
		cur += META_FIELD_LEN;
	}

	size_t n_written = 0;
	while(n_written < outbuf_len) {
		ssize_t n = write(self->metafd, outbuf, outbuf_len);
		if(n < 0) {
			// XXX Should we take this lying down, or keep trying?
			int new_errno = errno;
			free(outbuf);
			errno = new_errno;
			return STATUS_CHECK_ERRNO;
		}

		n_written += n;
	}

	free(outbuf);

	return 0;
}

int metafile_new_key_auto(metafile_t* self,
                          uint8_t master_key[crypto_secretbox_KEYBYTES],
                          uint8_t child_key[crypto_secretbox_KEYBYTES]) {
	// Use 10% of RAM up to 128MB for our memlimit
	long pages = sysconf(_SC_PHYS_PAGES);
	long page_size = sysconf(_SC_PAGE_SIZE);
	uint64_t mem_size = pages * page_size;

	uint32_t opslimit = crypto_pwhash_scryptsalsa208sha256_OPSLIMIT_INTERACTIVE;
	uint32_t memlimit = (uint32_t)fmin(mem_size*0.1, MAX_MEM_LIMIT);

	return metafile_new_key(self, opslimit, memlimit, master_key, child_key);
}

int metafile_new_key(metafile_t* self, uint32_t opslimit, uint32_t memlimit,
                     uint8_t master_key[crypto_secretbox_KEYBYTES],
                     uint8_t child_key[crypto_secretbox_KEYBYTES]) {
	metafield_t* field = metafile_append_key(self);
	if(field == NULL) { return STATUS_ERROR; }

	field->opslimit = opslimit;
	field->memlimit = memlimit;

	// Generate our nonce
	randombytes_buf(field->nonce, sizeof(field->nonce));

	// Encrypt the master key
	crypto_secretbox_easy(field->encrypted_key,
	                      master_key, crypto_secretbox_KEYBYTES,
	                      field->nonce, child_key);

	return 0;
}

void metafile_free(metafile_t* self) {
	close(self->metafd);
	exlock_release(self->lockpath);
	free(self->lockpath);
	free(self->metapath);
}
