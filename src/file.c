#include "fangfs.h"

#include <errno.h>
#include <unistd.h>
#include "util.h"
#include "file.h"

header_t header_parse(uint8_t buf[9]) {
	uint8_t* cur = buf;

	header_t header;
	header.version = (uint8_t)*cur;
	cur += sizeof(uint8_t);
	header.opslimit = u32_from_le_u32((uint32_t)(u32_from_bytes(cur)));
	cur += sizeof(uint32_t);
	header.memlimit = u32_from_le_u32((uint32_t)(u32_from_bytes(cur)));

	return header;
}

int fangfs_file_init(fangfs_file_t* self, fangfs_t* fs, int fd) {
	self->fs = fs;
	self->fd = fd;

	// Read in the header
	{
		uint8_t buf[9];
		ssize_t n_read = read(fd, buf, sizeof(buf));
		if(n_read < 0) { return errno; }
		if((size_t)n_read < sizeof(buf)) {
			return -1;
		}

		self->header = header_parse(buf);
	}

	return 0;
}
