#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "error.h"

void path_join(const char* p1, const char* p2, Buffer& outbuf) {
	size_t i = 0;
	while(p1[i] != 0) {
		if(i >= outbuf.buf_len) {
			buf_grow(outbuf, 0);
		}

		outbuf.buf[i] = p1[i];
		i += 1;
	}

	const size_t p1_len = i + 1;

	if((p1_len + 1) >= outbuf.buf_len) {
		buf_grow(outbuf, 0);
	}

	size_t p2i = 0;
	// Add a connecting path sep only if we need to
	if(i > 0) {
		if(p1[i-1] != '/' && p2[0] != '/') {
			outbuf.buf[i] = '/';
			i += 1;
		} else if(p1[i-1] == '/' && p2[0] == '/') {
			p2i += 1;
		}
	}

	while(p2[p2i] != 0) {
		outbuf.buf[i] = p2[p2i];
		i += 1;
		p2i += 1;

		if(i >= outbuf.buf_len) {
			buf_grow(outbuf, 0);
		}
	}

	outbuf.buf[i] = 0;
	outbuf.len = i;
}

char const* path_get_basename(const char* path) {
	int last_path_sep = -1;
	for(int i = 0; path[i] != '\0'; i += 1) {
		if(path[i] == '/') {
			last_path_sep = i;
		}
	}

	if(last_path_sep == -1) {
		return path;
	}

	return path + last_path_sep + 1;
}

void path_building_for_each(const Buffer& buf, std::function<void(const Buffer& buf)> f) {
	Buffer result;
	buf_copy(buf, result);

	// Skip the first character, since it can't be interesting
	size_t i = 1;
	while(i < buf.len) {
		if(buf[i] == '/') {
			result[i] = '\0';
			result.len = i;
			f(result);

			// Put things back
			result[i] = buf[i];
		}

		i += 1;
	}

	// Don't bother calling back on a trailing path separator
	if(i > 0 && buf[i-1] != '/') {
		result.len = buf.len;
		f(result);
	}
}

#define BASE32_SYMBOLS "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"
void base32_enc(const Buffer& input, Buffer& output) {
	if(input.len == 0) {
		buf_grow(output, 1);
		output.buf[0] = '\0';
		return;
	}

	buf_grow(output, (input.len * 1.6 + 2));

	size_t i = 1;
	size_t out_i = 0;
	int bits_left = 8;
	uint32_t group = input.buf[0];

	while(bits_left > 0 || i < input.len) {
		if(bits_left < 5) {
			// Load in the next group of bits
			if(i < input.len) {
				group <<= 8;
				group |= input.buf[i] & 0xff;
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
		output.buf[out_i] = BASE32_SYMBOLS[val];
		out_i += 1;
	}

	// Terminate the output string
	output.buf[out_i] = '\0';
	output.len = out_i + 1;
}

int base32_dec(const char* input, Buffer& output) {
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
			if(out_i >= output.buf_len) {
				buf_grow(output, 0);
			}

			output.buf[out_i] = group >> (bits_left - 8);
			out_i += 1;
			bits_left -= 8;
		}

		i += 1;
	}

	output.len = out_i;
	output.buf[out_i] = '\0';

	return 0;
}
