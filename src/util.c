#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "error.h"

void buf_init(buf_t* buf) {
	buf->buf = NULL;
	buf->buf_len = 0;
	buf->len = 0;
}

int buf_grow(buf_t* buf, size_t size) {
	// Never shrink the buffer.
	if(size != 0 && size < buf->buf_len) { return 0; }

	// If size is 0, then we're asked to use our best judgment.
	if(size == 0) {
		size = buf->buf_len * 2;
	}

	// If we currently have nothing, make our initial allocation 16 bytes.
	if(size == 0) {
		size = 16;
	}

	// Otherwise, use the provided size.
	uint8_t* newbuf = realloc(buf->buf, size);
	if(newbuf == NULL) { return STATUS_ERROR; }
	buf->buf_len = size;
	buf->buf = newbuf;

	return 0;
}

int buf_load_string(buf_t* buf, const char* str) {
	const size_t len = strlen(str) + 1;

	if(buf_grow(buf, buf->len) != 0) {
		return STATUS_ERROR;
	}
	buf->len = len - 1;
	memcpy(buf->buf, str, len);

	return 0;
}

char* buf_copy_string(buf_t* buf) {
	size_t len = strlen((char*)buf->buf);
	uint8_t* newbuf = malloc(len + 1); // Space for terminating null byte
	if(newbuf == NULL) { return NULL; }

	strcpy((char*)newbuf, (char*)buf->buf);
	buf->len = len;
	return (char*)newbuf;
}

void buf_free(buf_t* buf) {
	free(buf->buf);
}

int path_join(const char* p1, const char* p2, buf_t* outbuf) {
	size_t i = 0;
	while(p1[i] != 0) {
		if(i >= outbuf->buf_len) {
			if(buf_grow(outbuf, 0) != 0) {
				goto error;
			}
		}

		outbuf->buf[i] = p1[i];
		i += 1;
	}

	const size_t p1_len = i + 1;

	if((p1_len + 1) >= outbuf->buf_len) {
		if(buf_grow(outbuf, 0) != 0) {
			goto error;
		}
	}

	size_t p2i = 0;
	// Add a connecting path sep only if we need to
	if(i > 0) {
		if(p1[i-1] != '/' && p2[0] != '/') {
			outbuf->buf[i] = '/';
			i += 1;
		} else if(p1[i-1] == '/' && p2[0] == '/') {
			p2i += 1;
		}
	}

	while(p2[p2i] != 0) {
		outbuf->buf[i] = p2[p2i];
		i += 1;
		p2i += 1;

		if(i >= outbuf->buf_len) {
			if(buf_grow(outbuf, 0) != 0) {
				goto error;
			}
		}
	}

	outbuf->buf[i] = 0;
	outbuf->len = i;
	return 0;

	error: {
		// Keep people from accidentally using this buffer if they forget to
		// check the return code.
		outbuf->buf[0] = 0;
		outbuf->len = 0;
		return STATUS_ERROR;
	}
}

#define BASE32_SYMBOLS "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"
void base32_enc(const buf_t* input, buf_t* output) {
	if(input->len == 0) {
		buf_grow(output, 1);
		output->buf[0] = '\0';
		return;
	}

	buf_grow(output, (input->len * 1.6 + 2));

	size_t i = 1;
	size_t out_i = 0;
	int bits_left = 8;
	uint32_t group = input->buf[0];

	while(bits_left > 0 || i < input->len) {
		if(bits_left < 5) {
			// Load in the next group of bits
			if(i < input->len) {
				group <<= 8;
				group |= input->buf[i] & 0xff;
				bits_left += 8;
				i += 1;
			} else {
				int pad = 5 - bits_left;
				group <<= pad;
				bits_left += pad;
			}
		}

		// Compute the next output character
		int val = 0x1f & (group >> (bits_left - 5));
		bits_left -= 5;
		output->buf[out_i] = BASE32_SYMBOLS[val];
		out_i += 1;
	}

	// Terminate the output string
	output->buf[out_i] = '\0';
	output->len = out_i + 1;
}

int base32_dec(const char* input, buf_t* output) {
	size_t i = 0;
	size_t out_i = 0;
	uint32_t group = 0;
	int bits_left = 0;
	while(input[i] != '\0') {
		uint8_t ch = input[i];
		group <<= 5;

		if(ch >= 'A' && ch <= 'Z') {
			ch = (ch & 0x1f) - 1;
		} else if(ch >= '2' && ch <= '7') {
			ch -= '2' - 26;
		} else {
			return STATUS_ERROR;
		}

		group |= ch;
		bits_left += 5;
		if(bits_left >= 8) {
			if(out_i >= output->buf_len) {
				if(buf_grow(output, 0) != 0) {
					return STATUS_ERROR;
				}
			}

			output->buf[out_i] = group >> (bits_left - 8);
			out_i += 1;
			bits_left -= 8;
		}

		i += 1;
	}

	output->len = out_i;
	output->buf[out_i] = '\0';

	return 0;
}
