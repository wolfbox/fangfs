#include <errno.h>
#include <unistd.h>
#include "file.h"

int fangfs_file_init(fangfs_file_t* self, fangfs_t* fs, int fd) {
	self->fs = fs;
	self->fd = fd;

	return 0;
}
