#pragma once

#include <stddef.h>
#include <stdint.h>

/// A growable buffer type.
struct Buffer {
    Buffer(): buf(nullptr), buf_len(0), len(0) {}
    Buffer(Buffer&& other);
    uint8_t operator[](size_t i) const;
    uint8_t& operator[](size_t i);
    ~Buffer();

    /// The buffer.
    uint8_t* buf;

    /// The allocated buffer length.
    size_t buf_len;

    /// Usecase-defined logical content length.
    size_t len;
};

/// Grow a buffer to the given size, or double its size if minsize=0.
void buf_grow(Buffer& buf, size_t minsize);

/// Helper to copy a C-string into a buffer. The "len" property excludes the
/// terminating nul byte.
void buf_load_string(Buffer& buf, const char* str);

/// Copy src into dest
void buf_copy(const Buffer& src, Buffer& dest);

/// Helper to create an external copy of a C-string in a buffer.
char* buf_copy_string(Buffer& buf);

/// Free any memory associated with a buffer.
void buf_free(Buffer& buf);
