#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "util.h"
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

	self->opslimit = u32_from_le_u32((uint32_t)(u32_from_bytes(cur)));
	cur += sizeof(uint32_t);

	self->memlimit = u32_from_le_u32((uint32_t)(u32_from_bytes(cur)));
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

void metafile_init(metafile_t* self) {
	self->version = FANGFS_META_VERSION;
	self->n_keys = 0;
	memset(self->keys, 0, sizeof(self->keys));
}

int metafile_parse(metafile_t* self, int fd) {
	{
		ssize_t n_read = read(fd, &self->version, sizeof(self->version));
		if(n_read != 1) { return 1; }

		n_read = read(fd, &self->block_size, sizeof(self->block_size));
		if(n_read < (ssize_t)sizeof(self->block_size)) { return 1; }
		self->block_size = u32_from_le_u32(self->block_size);
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
				return 0;
			}
			n_read += n;
		}

		metafield_t* field = metafile_append_key(self);
		metafield_parse(field, buf);
	}

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
void metafile_free(metafile_t* self) {}
