#pragma once

#include <stdint.h>

int path_join(const char* p1, const char* p2, char* out, size_t out_len);

static inline uint32_t u32_from_le_u32(uint32_t x) {
	uint8_t* val = (uint8_t*)&x;
	return (val[0] << 0) | (val[1] << 8) | (val[2] << 16) | (val[3] << 24);
}

static inline uint32_t u32_from_bytes(uint8_t bytes[4]) {
	return *(uint32_t*)bytes;
}
