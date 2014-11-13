#include <errno.h>
#include <unistd.h>
#include "file.h"

int fang_file_init(FangFile& self, FangFS& fs, int fd) {
	self.fs = fs;
	self.fd = fd;

	return 0;
}
