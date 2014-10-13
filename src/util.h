#pragma once

#include <stdint.h>

/// A growable buffer type.
struct Buffer {
	/// The buffer.
	uint8_t* buf;

	/// The allocated buffer length.
	size_t buf_len;

	/// Usecase-defined logical content length.
	size_t len;

	Buffer(): buf(nullptr), buf_len(0), len(0) {}
	~Buffer();
};

/// Grow a buffer to the given size, or double its size if minsize=0. Returns
/// 0 on success.
int buf_grow(Buffer& buf, size_t minsize);

/// Helper to copy a C-string into a buffer. The "len" property excludes the
/// terminating nul byte. Returns 0 on success.
int buf_load_string(Buffer& buf, const char* str);

/// Helper to create an external copy of a C-string in a buffer. Returns NULL
/// on error.
char* buf_copy_string(Buffer& buf);

/// Free any memory associated with a buffer.
void buf_free(Buffer& buf);

/// Join two filesystem paths together. Returns 0 on success.
int path_join(const char* p1, const char* p2, Buffer& outbuf);

static inline uint32_t u32_from_bytes(const uint8_t bytes[4]) {
	return *(const uint32_t*)bytes;
}

void base32_enc(const Buffer& input, Buffer& output);
int base32_dec(const char* input, Buffer& output);

/// Convert a little-endian uint32_t into a native-endian uint32_t.
static inline uint32_t u32_from_le(uint32_t x) {
	uint8_t* val = (uint8_t*)&x;
	return (val[0] << 0) | (val[1] << 8) | (val[2] << 16) | (val[3] << 24);
}

#define FANGFS_LITTLE_ENDIAN
/*#if __LITTLE_ENDIAN
#define FANGFS_LITTLE_ENDIAN
#elif __BIG_ENDIAN
#define FANGFS_BIG_ENDIAN
#else
#error "Unknown platform endianness."
#endif*/

/// Convert a native-endian uint32_t value into little-endian.
static inline uint32_t u32_to_le(uint32_t x) {
#ifdef FANGFS_LITTLE_ENDIAN
	return x;
#else
	return (x<<24) | (x<<8 & 0xff0000) | (x>>8 & 0xff00) | (x>>24);
#endif
}
