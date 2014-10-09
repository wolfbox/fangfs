#pragma once

#define FUSE_USE_VERSION 26
#define _FILE_OFFSET_BITS 64

#include <sys/types.h>
#include <sys/stat.h>
#include <fuse.h>
#include <sodium.h>
#include "metafile.h"

typedef struct {
	metafile_t metafile;
	char master_key[crypto_secretbox_KEYBYTES];
	uint32_t blocksize;
	char const* source;
} fangfs_t;

int fangfs_fsinit(fangfs_t* self, const char* source);
void fangfs_fsclose(fangfs_t* self);

int fangfs_open(fangfs_t* self, const char* path, struct fuse_file_info* fi);
int fangfs_close(fangfs_t* self, struct fuse_file_info* fi);
int fangfs_getattr(fangfs_t* self, const char* path, struct stat* stbuf);
int fangfs_read(fangfs_t* self, char* buf, size_t size, off_t offset, \
                struct fuse_file_info* fi);
