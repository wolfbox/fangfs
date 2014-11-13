#pragma once

#include <sodium.h>
#include "fangfs.h"

#define BLOCK_HEADER_LEN (crypto_secretbox_NONCEBYTES)
#define BLOCK_OVERHEAD (BLOCK_HEADER_LEN + crypto_secretbox_MACBYTES)

struct FangFile {
	FangFile(FangFS& fang, int file): fs(fang), fd(file) {}
	FangFS& fs;
	int fd;
};

int fang_file_read(FangFile& self, off_t offset, size_t len, uint8_t* outbuf);
int fang_file_write(FangFile& self, off_t offset, size_t len, void* buf);
off_t fang_file_seek(FangFile& self, off_t offset, int whence);
