#pragma once

#include "fangfs.h"

typedef struct {
	fangfs_t* fs;
	int fd;
} fangfs_file_t;

int fangfs_file_init(fangfs_file_t* self, fangfs_t* fs, int fd);
