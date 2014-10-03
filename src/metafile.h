#pragma once

#include <stdint.h>
#include <sodium.h>

static const uint8_t FANGFS_META_VERSION = 0;

#define META_FIELD_LEN (sizeof(uint32_t)*2 + crypto_secretbox_NONCEBYTES + \
                        crypto_secretbox_KEYBYTES + crypto_secretbox_MACBYTES)

typedef struct {
	uint32_t opslimit; // scrypt CPU factor
	uint32_t memlimit; // scrypt memory factor
	char nonce[crypto_secretbox_NONCEBYTES];
	char encrypted_key[crypto_secretbox_KEYBYTES+crypto_secretbox_MACBYTES];
} metafield_t;

typedef struct {
	uint8_t version;
	metafield_t* keys;
} metafile_t;

/// Parse in a single field containing key information.
void metafield_parse(metafield_t* self, uint8_t inbuf[META_FIELD_LEN]);
void metafield_serialize(metafield_t* self, uint8_t outbuf[META_FIELD_LEN]);
