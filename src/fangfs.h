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
	uint8_t master_key[crypto_secretbox_KEYBYTES];
	char const* source;
} fangfs_t;

int fangfs_fsinit(fangfs_t* self, const char* source);
void fangfs_fsclose(fangfs_t* self);

int fangfs_mknod(fangfs_t* self, const char* path, mode_t m, dev_t d);
int fangfs_open(fangfs_t* self, const char* path, struct fuse_file_info* fi);
int fangfs_close(fangfs_t* self, struct fuse_file_info* fi);
int fangfs_getattr(fangfs_t* self, const char* path, struct stat* stbuf);
int fangfs_read(fangfs_t* self, char* buf, size_t size, off_t offset, \
                struct fuse_file_info* fi);
int fangfs_opendir(fangfs_t* self, const char* path, struct fuse_file_info* fi);
int fangfs_readdir(fangfs_t* self, const char* path, void* buf,
                        fuse_fill_dir_t filler, off_t offset,
                        struct fuse_file_info* fi);


// Filename utilities
int path_resolve(fangfs_t* self, const char* path, buf_t* outbuf);
int path_encrypt(fangfs_t* self, const char* orig, buf_t* outbuf);
int path_decrypt(fangfs_t* self, const char* orig, buf_t* outbuf);
