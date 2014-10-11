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

	buf_t in; buf_init(&in);
	buf_t out; buf_init(&out);
	for(size_t i = 0; i < sizeof(ASCII)/sizeof(char*); i += 1) {
		buf_load_string(&in, ASCII[i]);
		base32_enc(&in, &out);
		verify(strcmp((char*)out.buf, ENCODED[i]) == 0);

		base32_dec(ENCODED[i], &out);
		verify(strcmp((char*)out.buf, ASCII[i]) == 0);
	}

	buf_free(&in);
	buf_free(&out);
}

int main(void) {
	test();
	return 0;
}
