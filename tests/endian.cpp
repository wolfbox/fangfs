#include <string.h>
#include "test.h"
#include "../src/util.h"

void test_u32_from_le(void) {
	do_test();

	{
		uint32_t val = 0x0102;
		uint32_t le_val = u32_from_le(val);
#ifdef FANGFS_LITTLE_ENDIAN
		verify(le_val == val);
#elif FANGFS_BIG_ENDIAN
		verify(le_val != val);
#error "Neither FANGFS_LITTLE_ENDIAN or FANGFS_BIG_ENDIAN defined"
#endif
	}

	{
		union {
			uint32_t integer;
			uint8_t bytes[4];
		} raw_val;
		raw_val.bytes[0] = 0x02;
		raw_val.bytes[1] = 0x01;
		raw_val.bytes[2] = 0x00;
		raw_val.bytes[3] = 0x00;
		verify(u32_from_le(raw_val.integer) == 258);
	}
}

void test_u32_to_le(void) {
	do_test();

	{
		uint32_t val = 0x0102;
		uint32_t le_val = u32_to_le(val);
#ifdef FANGFS_LITTLE_ENDIAN
		verify(le_val == val);
#elif FANGFS_BIG_ENDIAN
		verify(le_val != val);
#endif
	}

	{
		uint8_t bytes[4] = { 0x02, 0x01, 0x00, 0x00 };
		uint32_t val = u32_to_le(0x0102);
		verify(memcmp(bytes, &val, sizeof(val)) == 0);
	}
}

int main(void) {
	test_u32_from_le();
	test_u32_to_le();
	return 0;
}
