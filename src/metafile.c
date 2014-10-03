#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "util.h"
#include "metafile.h"

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
