#pragma once

#include "fangfs.h"
#include <stdint.h>

typedef struct {
	uint8_t version;
	uint32_t opslimit;
	uint32_t memlimit;
} header_t;

header_t header_parse(uint8_t buf[9]);

typedef struct {
	fangfs_t* fs;
	header_t header;
	int fd;
} fangfs_file_t;

int fangfs_file_init(fangfs_file_t* self, fangfs_t* fs, int fd);
