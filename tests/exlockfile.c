#include "test.h"
#include "../src/exlockfile.h"

void test_simple(void) {
	do_test();

	int status = exlock_obtain("test-lock");
	verify(status == 0);

	verify(!exlock_try_obtain("test-lock"));

	verify(exlock_release("test-lock") == 0);

	verify(exlock_try_obtain("test-lock"));
	verify(exlock_release("test-lock") == 0);
}

void test_illegal(const char* selfpath) {
	do_test();
	verify(!exlock_try_obtain(selfpath));
}

int main(int argc, char** argv) {
	test_simple();
	test_illegal(argv[0]);

	return 0;
}
