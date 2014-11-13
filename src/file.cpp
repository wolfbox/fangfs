#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <array>
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
		errno = -EIO;
		return -1;
	}

	// Extract the nonce
	uint8_t nonce[crypto_secretbox_NONCEBYTES];
	memcpy(nonce, ciphertext.buf, sizeof(nonce));

	// Slide the remainder back over the nonce so it begins the buffer
	memmove(ciphertext.buf, ciphertext.buf+sizeof(nonce), ciphertext.len-sizeof(nonce));

	// Decrypt
	const int status = buf_decrypt(ciphertext, nonce, self.fs.master_key, outbuf);
	if(status != 0) {
		// Tampering detected
		errno = -EIO;
		return -1;
	}

	return outbuf.len;
}

int fang_file_read(FangFile& self, off_t offset, size_t len, uint8_t* outbuf) {
	const uint64_t start_block_n = get_block_number(self, offset);
	const uint64_t end_block_n = get_block_number(self, offset+len);

	Buffer tmpbuf;
	size_t outi = 0;
	for(uint64_t i = start_block_n; i <= end_block_n; i += 1) {
		const ssize_t n = block_read(self, i, tmpbuf);
		if(n < 0) {
			return errno;
		}
		memcpy(outbuf+outi, tmpbuf.buf, n);
		i += n;
		outi += n;
	}

	return 0;
}

int fang_file_write(FangFile& self, off_t offset, size_t len, void* buf) {
	return -1;
}

off_t fang_file_seek(FangFile& self, off_t offset, int whence) {
	return lseek(self.fd, translate_offset(self, offset), whence);
}
