#include <string.h>
#include "../src/util.h"
#include "test.h"

void simple1(Buffer& buf) {
	return path_join("/foo/barbar", "/baz", buf);
}
#define SIMPLE1_ANSWER "/foo/barbar/baz"

void simple2(Buffer& buf) {
	return path_join("/foo/bar/", "baz", buf);
}
#define SIMPLE2_ANSWER "/foo/bar/baz"

void simple3(Buffer& buf) {
	return path_join("/foo/bar/", "/baz12345", buf);
}
#define SIMPLE3_ANSWER "/foo/bar/baz12345"

void simple4(Buffer& buf) {
	return path_join("/home/user/", "__FANGFS_META.lock", buf);
}
#define SIMPLE4_ANSWER "/home/user/__FANGFS_META.lock"

void empty_lhs(Buffer& buf) {
	return path_join("", "/foobar", buf);
}
#define EMPTY_LHS_ANSWER "/foobar"

void test_ok(void(*f)(Buffer&), const char* answer) {
	do_test();

	Buffer buf;
	f(buf);
	verify(strcmp(answer, (char*)buf.buf) == 0);
}

void test_empty_lhs(void) {
	do_test();

	Buffer buf;
	empty_lhs(buf);
	verify(strcmp(EMPTY_LHS_ANSWER, (char*)buf.buf) == 0);
}

void path_building_for_each(void) {
	do_test();

	const char* correct[] = {"/home", "/home/foo", "/home/foo/things"};
	Buffer src;
	{
		size_t i = 0;
		buf_load_string(src, "/home/foo/things");
		path_building_for_each(src, [&](const Buffer& cur) {
			verify(strcmp(reinterpret_cast<const char*>(cur.buf), correct[i]) == 0);
			i += 1;
		});
		verify(i == 3);
	}

	{
		size_t i = 0;
		buf_load_string(src, "/home/foo/things/");
		path_building_for_each(src, [&](const Buffer& cur) {
			verify(strcmp(reinterpret_cast<const char*>(cur.buf), correct[i]) == 0);
			i += 1;
		});
		verify(i == 3);
	}
}

int main(void) {
	test_ok(simple1, SIMPLE1_ANSWER);
	test_ok(simple2, SIMPLE2_ANSWER);
	test_ok(simple3, SIMPLE3_ANSWER);
	test_ok(simple4, SIMPLE4_ANSWER);
	test_empty_lhs();
	path_building_for_each();

	return 0;
}
