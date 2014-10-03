#include "test.h"
#include "../src/file.h"

void test_parse(void) {
	do_test();

	uint8_t buf[9] = { 0x01,
	                   0x01, 0x02, 0x00, 0x00,
	                   0x01, 0x02, 0x00, 0x00 };
	header_t header = header_parse(buf);
	verify(header.version == 1);
	verify(header.opslimit == 513);
	verify(header.memlimit == 513);
}

int main(void) {
	test_parse();
	return 0;
}
