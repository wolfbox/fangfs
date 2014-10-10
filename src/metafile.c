#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include "util.h"
#include "exlockfile.h"
#include "metafile.h"

// Don't use more than 128M of memory in our kdf when using automatic settings.
#define MAX_MEM_LIMIT 1024 * 1024 * 1024

static metafield_t* metafile_append_key(metafile_t* self) {
	if(self->n_keys >= METAFILE_MAX_KEYS) {
		return NULL;
	}

	metafield_t* field = &self->keys[self->n_keys];
	self->n_keys += 1;

	memset(field, 0, sizeof(metafield_t));
	return field;
}

void metafield_parse(metafield_t* self, uint8_t inbuf[META_FIELD_LEN]) {
	uint8_t* cur = inbuf;

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

int metafile_init(metafile_t* self, const char* sourcepath) {
	int status = 0;

	self->version = FANGFS_META_VERSION;
	self->n_keys = 0;
	memset(self->keys, 0, sizeof(self->keys));

	// Create the path to the metafile
	buf_t path_buf;
	buf_init(&path_buf);
	if(path_join(sourcepath, METAFILE_NAME, &path_buf) != 0) {
		status = 1;
		goto cleanup;
	}
	self->metapath = buf_copy_string(&path_buf);
	if(self->metapath == NULL) {
		status = 1;
		goto cleanup;
	}

	// Create the path to the lockfile
	if(path_join(sourcepath, METAFILE_LOCK, &path_buf) != 0) {
		status = 1;
		goto cleanup;
	}
	self->lockpath = buf_copy_string(&path_buf);
	if(self->lockpath == NULL) {
		status = 1;
		goto cleanup;
	}

	cleanup:
		buf_free(&path_buf);
		return status;
}

int metafile_parse(metafile_t* self) {
	int status = exlock_obtain(self->lockpath);
	if(status != 0) { return status; }

	int fd = open(self->metapath, O_RDONLY);
	if(fd < 0) {
		exlock_release(self->lockpath);
		return 1;
	}

	{
		ssize_t n_read = read(fd, &self->version, sizeof(self->version));
		if(n_read != 1) {
			status = 1;
			goto cleanup;
		}

		n_read = read(fd, &self->block_size, sizeof(self->block_size));
		if(n_read < (ssize_t)sizeof(self->block_size)) {
			status = 1;
			goto cleanup;
		}
		self->block_size = u32_from_le(self->block_size);
	}

	// Read records until EOF
	while(1) {
		uint8_t buf[META_FIELD_LEN];
		ssize_t n_read = 0;

		// Read the current record until either EOF or it's finished.
		while(n_read < (ssize_t)META_FIELD_LEN) {
			ssize_t n = read(fd, buf, (META_FIELD_LEN-n_read));
			if(n == 0) {
				// EOF
				status = 0;
				goto cleanup;
			}
			n_read += n;
		}

		metafield_t* field = metafile_append_key(self);
		metafield_parse(field, buf);
	}

	cleanup:
		exlock_release(self->lockpath);
		close(fd);
		return status;
}

int metafile_write(metafile_t* self) {
	// XXX We should back up the metafile BEFORE overwriting it.
	// Just in case.
	// Obtain our lock
	int status = exlock_obtain(self->lockpath);
	if(status != 0) { return status; }

	int fd = open(self->metapath, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
	if(fd < 0) {
		exlock_release(self->lockpath);
		return 1;
	}

	size_t outbuf_len = sizeof(self->version) +
	                    sizeof(self->block_size) +
	                    (META_FIELD_LEN * self->n_keys);
	uint8_t* outbuf = malloc(outbuf_len);
	uint8_t* cur = outbuf;

	memcpy(cur, &self->version, sizeof(self->version));
	cur += sizeof(self->version);

	uint32_t block_size = u32_to_le(self->block_size);
	memcpy(cur, &block_size, sizeof(block_size));
	cur += sizeof(block_size);

	for(size_t i = 0; i < self->n_keys; i += 1) {
		metafield_serialize(&self->keys[i], cur);
		cur += META_FIELD_LEN;
	}

	size_t n_written = 0;
	while(n_written < outbuf_len) {
		ssize_t n = write(fd, outbuf, outbuf_len);
		if(n < 0) {
			// XXX Should we take this lying down, or keep trying?
			status = errno;
			goto cleanup;
		}

		n_written += n;
	}

	cleanup:
		free(outbuf);
		exlock_release(self->lockpath);
		close(fd);
		return status;
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
	if(field == NULL) { return 1; }

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

// No-op right now, but down the road this may change.
void metafile_free(metafile_t* self) {
	free(self->lockpath);
	free(self->metapath);
}
