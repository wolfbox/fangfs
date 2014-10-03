#include <string.h>
#include "../src/util.h"
#include "test.h"

int simple1(char* buf, size_t buf_size) {
	return path_join("/foo/bar", "/baz", buf, buf_size);
}

int simple2(char* buf, size_t buf_size) {
	return path_join("/foo/bar/", "baz", buf, buf_size);
}

int simple3(char* buf, size_t buf_size) {
	return path_join("/foo/bar/", "/baz", buf, buf_size);
}

void test_ok(int(*f)(char*, size_t)) {
	do_test();

	char buf[13];
	int status = f(buf, sizeof(buf));
	verify(strcmp("/foo/bar/baz", buf) == 0);
	verify(status == 0);
}

void test_overflow(int(*f)(char*, size_t)) {
	do_test();

	for(int i = 0; i < 13; i += 1) {
		char buf[i];
		int status = f(buf, sizeof(buf));
		verify(status == 1);
	}
}

void test_null_buf(void) {
	do_test();

	int status = simple1(NULL, 0);
	verify(status == 1);
}

int main(void) {
	test_ok(simple1);
	test_ok(simple2);
	test_ok(simple3);
	test_overflow(simple1);
	test_overflow(simple2);
	test_overflow(simple3);
	test_null_buf();

	return 0;
}
