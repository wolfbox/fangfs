#pragma once

#include "fangfs.h"

struct FangFile {
	FangFS& fs;
	int fd;
};

int fang_file_init(FangFile& self, FangFS& fs, int fd);
