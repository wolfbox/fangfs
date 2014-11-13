#pragma once

#define FUSE_USE_VERSION 26
#define _FILE_OFFSET_BITS 64

#include <sys/types.h>
#include <sys/stat.h>
#include <fuse.h>
#include <sodium.h>
#include "metafile.h"

struct FangFS {
	Metafile metafile;
	uint8_t master_key[crypto_secretbox_KEYBYTES];
	char const* source;
};

int fangfs_fsinit(FangFS& self, const char* source);
void fangfs_fsclose(FangFS& self);

int fangfs_mknod(FangFS& self, const char* path, mode_t m, dev_t d);
int fangfs_open(FangFS& self, const char* path, struct fuse_file_info* fi);
int fangfs_close(FangFS& self, struct fuse_file_info* fi);
int fangfs_getattr(FangFS& self, const char* path, struct stat* stbuf);
int fangfs_read(FangFS& self, char* buf, size_t size, off_t offset, \
                struct fuse_file_info* fi);
int fangfs_mkdir(FangFS& self, const char* path, mode_t mode);
int fangfs_opendir(FangFS& self, const char* path, struct fuse_file_info* fi);
int fangfs_readdir(FangFS& self, const char* path, void* buf,
                        fuse_fill_dir_t filler, off_t offset,
                        struct fuse_file_info* fi);


// Filename utilities
int path_resolve(FangFS& self, const char* path, Buffer& outbuf);
int path_encrypt(FangFS& self, const char* orig, Buffer& outbuf);
int path_decrypt(FangFS& self, const char* orig, Buffer& outbuf);
