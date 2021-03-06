#include <string.h>
#include "test.h"
#include "../src/util.h"

const char* ASCII[] = {
	"",
	"f",
	"fo",
	"foo",
	"foob",
	"fooba",
	"foobar" };

const char* ENCODED[] = {
	"",
	"MY",
	"MZXQ",
	"MZXW6",
	"MZXW6YQ",
	"MZXW6YTB",
	"MZXW6YTBOI" };

void test() {
	do_test();

	Buffer in;
	Buffer out;
	for(size_t i = 0; i < sizeof(ASCII)/sizeof(char*); i += 1) {
		buf_load_string(in, ASCII[i]);
		base32_enc(in, out);
		verify(strcmp((char*)out.buf, ENCODED[i]) == 0);

		base32_dec(ENCODED[i], out);
		verify(strcmp((char*)out.buf, ASCII[i]) == 0);
	}
}

int main(void) {
	test();
	return 0;
}
