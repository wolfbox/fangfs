#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <algorithm>
#include "Buffer.h"
#include "BufferEncryption.h"
#include "file.h"

/// Return the number of the block containing the given byte offset.
static inline uint64_t get_block_number(const FangFile& self, off_t offset) {
	return (offset / self.fs.metafile.block_size) *
	       self.fs.metafile.block_size;
}

/// Translate the logical decrypted file offset into an underlying offset,
/// taking into account the block headers MACs.
static inline off_t translate_offset(const FangFile& self, off_t offset) {
	const uint64_t block_n = get_block_number(self, offset);
	return (block_n * (BLOCK_OVERHEAD)) + offset;
}

static ssize_t block_read(FangFile& self, uint64_t block_n, Buffer& outbuf) {
	if(outbuf.len > (self.fs.metafile.block_size - BLOCK_OVERHEAD)) {
		errno = EINVAL;
		return -1;
	}

	const off_t offset = block_n * self.fs.metafile.block_size;
	const off_t seek_status = fang_file_seek(self, offset, SEEK_SET);
	if(seek_status == (off_t)-1) {
		return -1;
	}

	const size_t goal_n = self.fs.metafile.block_size + BLOCK_OVERHEAD;
	Buffer ciphertext;
	buf_grow(ciphertext, goal_n);

	size_t total_read = 0;
	uint8_t* cur = ciphertext.buf;
	while(total_read < goal_n) {
		const ssize_t n = read(self.fd, cur, goal_n);
		if(n == 0) {
			break;
		} else if(n < 0) {
			fprintf(stderr, "Error03: fd %d\n", self.fd);
			return -1;
		}

		total_read += n;
		cur += n;
	}
	ciphertext.len = total_read;

	// Make sure there's enough here to work with
	if(ciphertext.len < BLOCK_OVERHEAD) {
		// Empty virtual files have empty physical files
		if(ciphertext.len == 0) { return 0; }
		errno = EIO;
		return -1;
	}

	// Extract the nonce
	uint8_t nonce[crypto_secretbox_NONCEBYTES];
	memcpy(nonce, ciphertext.buf, sizeof(nonce));

	// Slide the remainder back over the nonce so it begins the buffer
	memmove(ciphertext.buf, ciphertext.buf+sizeof(nonce), ciphertext.len-sizeof(nonce));
	ciphertext.len -= sizeof(nonce);

	// Decrypt
	const int status = buf_decrypt(ciphertext, nonce, self.fs.master_key, outbuf);
	if(status != 0) {
		// Tampering detected
		errno = EIO;
		fprintf(stderr, "Error05\n");
		return -1;
	}

	return outbuf.len;
}

static ssize_t block_write(FangFile& self, uint64_t block_n, const Buffer& inbuf) {
	if(inbuf.len > (self.fs.metafile.block_size - BLOCK_OVERHEAD)) {
		errno = EINVAL;
		return -1;
	}

	const off_t offset = block_n * self.fs.metafile.block_size;

	const off_t seek_status = fang_file_seek(self, offset, SEEK_SET);
	if(seek_status == (off_t)-1) {
		return -1;
	}

	const size_t goal_n = self.fs.metafile.block_size;
	Buffer ciphertext;
	buf_grow(ciphertext, goal_n);

	uint8_t nonce[crypto_secretbox_NONCEBYTES];
	randombytes_buf(nonce, sizeof(nonce));

	buf_encrypt(inbuf, nonce, self.fs.master_key, ciphertext);

	memmove(ciphertext.buf+BLOCK_HEADER_LEN, ciphertext.buf, ciphertext.len);
	ciphertext.len += BLOCK_HEADER_LEN;
	memcpy(ciphertext.buf, nonce, sizeof(nonce));

	size_t total_written = 0;
	uint8_t* cur = ciphertext.buf;
	while(total_written < ciphertext.len) {
		const ssize_t n = write(self.fd, cur, ciphertext.len);
		if(n < 0) {
			return -1;
		}

		total_written += n;
		cur += n;
	}

	return total_written;
}

int fang_file_read(FangFile& self, off_t offset, size_t len, uint8_t* outbuf) {
	const uint64_t start_block_n = get_block_number(self, offset);
	const uint64_t end_block_n = get_block_number(self, offset+len);

	Buffer tmpbuf;
	size_t outi = 0;
	for(uint64_t i = start_block_n; i <= end_block_n; i += 1) {
		const ssize_t n = block_read(self, i, tmpbuf);
		if(n < 0) {
			return -errno;
		}

		memcpy(outbuf+outi, tmpbuf.buf, n);
		outi += n;
	}

	return static_cast<int>(outi);
}

int fang_file_write(FangFile& self, off_t offset, size_t len, const uint8_t* buf) {
	const uint64_t start_block_n = get_block_number(self, offset);
	const uint64_t end_block_n = get_block_number(self, offset+len);

	Buffer newblock;
	buf_grow(newblock, self.fs.metafile.block_size);

	size_t srci = 0;
	for(uint64_t i = start_block_n; i <= end_block_n; i += 1) {
		const ssize_t n_read = block_read(self, i, newblock);
		if(n_read < 0) {
			return -errno;
		}

		const size_t delta = (offset / self.fs.metafile.block_size) *
		                     self.fs.metafile.block_size;
		const size_t outi = offset - delta;
		const size_t n = std::min(len, (self.fs.metafile.block_size - delta));

		memcpy(newblock.buf+outi, buf+srci, n);
		newblock.len = n;
		srci += n;

		const int status = block_write(self, i, newblock);
		if(status < 0) {
			return -errno;
		}
	}

	return srci;
}

off_t fang_file_seek(FangFile& self, off_t offset, int whence) {
	return lseek(self.fd, translate_offset(self, offset), whence);
}
