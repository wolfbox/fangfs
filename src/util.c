#include <stdlib.h>
#include <string.h>
#include "util.h"

void buf_init(buf_t* buf) {
	buf->buf = NULL;
	buf->buf_len = 0;
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
	if(newbuf == NULL) { return 1; }
	buf->buf = newbuf;

	return 0;
}

char* buf_copy_string(buf_t* buf) {
	size_t len = strlen((char*)buf->buf);
	uint8_t* newbuf = malloc(len + 1); // Space for terminating null byte
	if(newbuf == NULL) { return NULL; }

	strcpy((char*)newbuf, (char*)buf->buf);
	return (char*)newbuf;
}

void buf_free(buf_t* buf) {
	free(buf->buf);
}

int path_join(const char* p1, const char* p2, buf_t* outbuf) {
	if(outbuf == NULL) return 1;

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
	if(p1[i-1] != '/' && p2[0] != '/') {
		outbuf->buf[i] = '/';
		i += 1;
	} else if(p1[i-1] == '/' && p2[0] == '/') {
		p2i += 1;
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
	return 0;

	error: {
		// Keep people from accidentally using this buffer if they forget to
		// check the return code.
		outbuf->buf[0] = 0;
		return 1;
	}
}
