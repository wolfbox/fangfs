#include "Buffer.h"
#include <stdlib.h>
#include <string.h>
#include "error.h"

Buffer::Buffer(Buffer&& other): buf(other.buf), buf_len(other.buf_len), len(other.len) {
    other.buf = nullptr;
    other.buf_len = 0;
    other.len = 0;
}

Buffer::~Buffer() { buf_free(*this); }

void buf_grow(Buffer& buf, size_t size) {
    // Never shrink the buffer.
    if(size != 0 && size < buf.buf_len) { return; }

    // If size is 0, then we're asked to use our best judgment.
    if(size == 0) {
        size = buf.buf_len * 2;
    }

    // If we currently have nothing, make our initial allocation 16 bytes.
    if(size == 0) {
        size = 16;
    }

    // Otherwise, use the provided size.
    uint8_t* newbuf = (uint8_t*)realloc(buf.buf, size);
    if(newbuf == NULL) { throw AllocationError(); }
    buf.buf_len = size;
    buf.buf = newbuf;
}

void buf_load_string(Buffer& buf, const char* str) {
    const size_t len = strlen(str) + 1;

    buf_grow(buf, buf.len);
    buf.len = len - 1;
    memcpy(buf.buf, str, len);
}

char* buf_copy_string(Buffer& buf) {
    size_t len = strlen((char*)buf.buf);
    uint8_t* newbuf = (uint8_t*)malloc(len + 1); // Space for terminating null byte
    if(newbuf == nullptr) { throw AllocationError(); }

    strcpy((char*)newbuf, (char*)buf.buf);
    buf.len = len;
    return (char*)newbuf;
}

void buf_free(Buffer& buf) {
    free(buf.buf);
    buf.buf_len = 0;
    buf.len = 0;
    buf.buf = nullptr;
}
