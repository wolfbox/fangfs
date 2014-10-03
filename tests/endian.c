#include "test.h"
#include "../src/util.h"

void test_u32_from_le_u32(void) {
	do_test();

	{
		uint32_t val = 0x0102;
		verify(u32_from_le_u32(val) == 0x0102);
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
		verify(u32_from_le_u32(raw_val.integer) == 258);
	}
}

int main(void) {
	test_u32_from_le_u32();
}
