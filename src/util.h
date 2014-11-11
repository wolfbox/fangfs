#pragma once

#include <stdint.h>
#include <functional>
#include "Buffer.h"

/// Join two filesystem paths together. Returns 0 on success.
int path_join(const char* p1, const char* p2, Buffer& outbuf);

/// Call f("foo"), f("foo/bar"), f("foo/bar/baz"), etc.
void path_building_for_each(const Buffer& buf, std::function<void(const Buffer& buf)> f);

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
