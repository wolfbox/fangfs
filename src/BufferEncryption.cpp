#include "BufferEncryption.h"

void buf_encrypt(const Buffer& inbuf,
                 const uint8_t nonce[crypto_secretbox_NONCEBYTES],
                 const uint8_t key[crypto_secretbox_KEYBYTES], Buffer& outbuf) {
	buf_grow(outbuf, inbuf.len + crypto_secretbox_MACBYTES);
	crypto_secretbox_easy(outbuf.buf, inbuf.buf, inbuf.len,
	                      nonce, key);
	outbuf.len = inbuf.len + crypto_secretbox_MACBYTES;
}

int buf_decrypt(const Buffer& inbuf,
                const uint8_t nonce[crypto_secretbox_NONCEBYTES],
                const uint8_t key[crypto_secretbox_KEYBYTES], Buffer& outbuf) {
    const size_t outlen = inbuf.len - crypto_secretbox_MACBYTES;

    buf_grow(outbuf, outlen);
	int result = crypto_secretbox_open_easy(outbuf.buf,
	                                        inbuf.buf,
	                                        inbuf.len,
	                                        nonce,
	                                        key);
	if(result != 0) {
		outbuf.len = 0;
		if(outbuf.buf_len > 0) {
			outbuf.buf[0] = '\0';
		}
		return result;
	}

	outbuf.len = outlen;

	return result;
}
