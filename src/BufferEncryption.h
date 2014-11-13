#pragma once

#include <stdint.h>
#include <sodium.h>
#include "Buffer.h"

void buf_encrypt(const Buffer& inbuf,
                 const uint8_t nonce[crypto_secretbox_NONCEBYTES],
                 const uint8_t key[crypto_secretbox_KEYBYTES], Buffer& outbuf);

/// Returns 0 on success
int buf_decrypt(const Buffer& inbuf,
                const uint8_t nonce[crypto_secretbox_NONCEBYTES],
                const uint8_t key[crypto_secretbox_KEYBYTES], Buffer& outbuf);
