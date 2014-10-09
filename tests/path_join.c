#include <string.h>
#include "../src/util.h"
#include "test.h"

int simple1(buf_t* buf) {
	return path_join("/foo/bar", "/baz", buf);
}

int simple2(buf_t* buf) {
	return path_join("/foo/bar/", "baz", buf);
}

int simple3(buf_t* buf) {
	return path_join("/foo/bar/", "/baz", buf);
}

void test_ok(int(*f)(buf_t*)) {
	do_test();

	buf_t buf;
	buf_init(&buf);
	int status = f(&buf);
	verify(strcmp("/foo/bar/baz", (char*)buf.buf) == 0);
	verify(status == 0);

	buf_free(&buf);
}

void test_null_buf(void) {
	do_test();

	int status = simple1(NULL);
	verify(status == 1);
}

int main(void) {
	test_ok(simple1);
	test_ok(simple2);
	test_ok(simple3);
	test_null_buf();

	return 0;
}
