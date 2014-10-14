#include <string.h>
#include "../src/util.h"
#include "test.h"

int simple1(Buffer& buf) {
	return path_join("/foo/barbar", "/baz", buf);
}
#define SIMPLE1_ANSWER "/foo/barbar/baz"

int simple2(Buffer& buf) {
	return path_join("/foo/bar/", "baz", buf);
}
#define SIMPLE2_ANSWER "/foo/bar/baz"

int simple3(Buffer& buf) {
	return path_join("/foo/bar/", "/baz12345", buf);
}
#define SIMPLE3_ANSWER "/foo/bar/baz12345"

int simple4(Buffer& buf) {
	return path_join("/home/user/", "__FANGFS_META.lock", buf);
}
#define SIMPLE4_ANSWER "/home/user/__FANGFS_META.lock"

int empty_lhs(Buffer& buf) {
	return path_join("", "/foobar", buf);
}
#define EMPTY_LHS_ANSWER "/foobar"

void test_ok(int(*f)(Buffer&), const char* answer) {
	do_test();

	Buffer buf;
	int status = f(buf);
	verify(strcmp(answer, (char*)buf.buf) == 0);
	verify(status == 0);
}

void test_empty_lhs(void) {
	do_test();

	Buffer buf;
	int status = empty_lhs(buf);
	verify(strcmp(EMPTY_LHS_ANSWER, (char*)buf.buf) == 0);
	verify(status == 0);

}

int main(void) {
	test_ok(simple1, SIMPLE1_ANSWER);
	test_ok(simple2, SIMPLE2_ANSWER);
	test_ok(simple3, SIMPLE3_ANSWER);
	test_ok(simple4, SIMPLE4_ANSWER);
	test_empty_lhs();

	return 0;
}