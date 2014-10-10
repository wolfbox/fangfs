#pragma once

#include <stdint.h>

/// A growable buffer type.
typedef struct {
	/// The buffer.
	uint8_t* buf;

	/// The allocated buffer length.
	size_t buf_len;
} buf_t;

/// Initialize an empty buffer.
void buf_init(buf_t* buf);

/// Grow a buffer to the given size, or double its size if minsize=0. Returns
/// 0 on success, and 1 on error.
int buf_grow(buf_t* buf, size_t minsize);

/// Helper to create an external copy of a C-string in a buffer. Returns NULL
/// on error.
char* buf_copy_string(buf_t* buf);

/// Free any memory associated with a buffer.
void buf_free(buf_t* buf);

int path_join(const char* p1, const char* p2, buf_t* outbuf);

static inline uint32_t u32_from_le_u32(uint32_t x) {
	uint8_t* val = (uint8_t*)&x;
	return (val[0] << 0) | (val[1] << 8) | (val[2] << 16) | (val[3] << 24);
}

static inline uint32_t u32_from_bytes(uint8_t bytes[4]) {
	return *(uint32_t*)bytes;
}
