#pragma once

#include <stdint.h>
#include <sodium.h>
#include "util.h"

static const uint8_t FANGFS_META_VERSION = 0;

#define METAFILE_LOCK "__FANGFS_META.lock"
#define METAFILE_NAME "__FANGFS_META"
#define META_FIELD_LEN (sizeof(uint32_t)*2 + crypto_secretbox_NONCEBYTES + \
                        crypto_secretbox_KEYBYTES + crypto_secretbox_MACBYTES)

typedef struct {
	/// scrypt CPU factor
	uint32_t opslimit;

	/// scrypt memory factor
	uint32_t memlimit;

	/// The random nonce
	uint8_t nonce[crypto_secretbox_NONCEBYTES];

	/// The master key encrypted with the child key
	uint8_t encrypted_key[crypto_secretbox_KEYBYTES+crypto_secretbox_MACBYTES];
} metafield_t;

#define METAFILE_MAX_KEYS 8
typedef struct {
	int metafd;
	char* metapath;
	char* lockpath;

	uint8_t version;
	uint32_t block_size;

	uint8_t filename_nonce[crypto_secretbox_xsalsa20poly1305_NONCEBYTES];

	size_t n_keys;
	metafield_t keys[METAFILE_MAX_KEYS];
} metafile_t;

#if crypto_secretbox_xsalsa20poly1305_NONCEBYTES != 24
#    error "Weird nonce length"
#endif

/// Parse in a single field containing key information.
void metafield_parse(metafield_t* self, const uint8_t inbuf[META_FIELD_LEN]);

/// Dump this key information field into a buffer.
void metafield_serialize(metafield_t* self, uint8_t outbuf[META_FIELD_LEN]);

/// Dump a key information field out into a buffer.
void metafield_serialize(metafield_t* self, uint8_t outbuf[META_FIELD_LEN]);

/// Initialize an empty metafile. The provided metapath is copied internally.
/// Returns 0 if the metafile is created, and 1 if it already existed.
int metafile_init(metafile_t* self, const char* metapath);

/// Parse in an existing metafile.
int metafile_parse(metafile_t* self);

/// Dump the metafile out to disk.
int metafile_write(metafile_t* self);

/// Create a new key.
int metafile_new_key(metafile_t* self, uint32_t opslimit, uint32_t memlimit,
                     uint8_t master_key[crypto_secretbox_KEYBYTES],
                     uint8_t child_key[crypto_secretbox_KEYBYTES]);

/// Wrapper to initialize a new key using automatic settings.
int metafile_new_key_auto(metafile_t* self,
                          uint8_t master_key[crypto_secretbox_KEYBYTES],
                          uint8_t child_key[crypto_secretbox_KEYBYTES]);

void metafile_free(metafile_t* self);
