//
// Created by David on 04/10/2020.
//

#include "buffer.h"
#include <stdarg.h>
#include <stdio.h>

Buffer *buf_vprintf(const char *format, va_list ap)
{
    va_list ap2;
    va_copy(ap2, ap);

    int len1 = vsnprintf(nullptr, 0, format, ap);
    assert(len1 >= 0);

    size_t required_size = len1 + 1;

    Buffer* buf = buf_alloc_fixed(len1);

    int len2 = vsnprintf(buf_ptr(buf), required_size, format, ap2);
    assert(len2 == len1);

    va_end(ap2);

    return buf;
}

Buffer* buf_sprintf(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    Buffer* result = buf_vprintf(format, ap);
    va_end(ap);
    return result;
}

void buf_appendf(Buffer *buf, const char *format, ...) {
    assert(buf->length);
    va_list ap, ap2;
    va_start(ap, format);
    va_copy(ap2, ap);

    int len1 = vsnprintf(nullptr, 0, format, ap);
    assert(len1 >= 0);

    size_t required_size = len1 + 1;

    size_t orig_len = buf_len(buf);

    buf_resize(buf, orig_len + len1);

    int len2 = vsnprintf(buf_ptr(buf) + orig_len, required_size, format, ap2);
    assert(len2 == len1);

    va_end(ap2);
    va_end(ap);
}

bool buf_eql_str(Buffer* buf, const char* str) {
    assert(buf->length);
    return buf_eql_mem(buf, str, strlen(str));
}

// these functions are not static inline so they can be better used as template parameters
bool buf_eql_buf(Buffer *buf, Buffer *other) {
    return buf_eql_mem(buf, buf_ptr(other), buf_len(other));
}

uint32_t buf_hash(Buffer *buf) {
    assert(buf->length);
    size_t interval = buf->length / 256;
    if (interval == 0)
        interval = 1;
    // FNV 32-bit hash
    uint32_t h = 2166136261;
    for (size_t i = 0; i < buf_len(buf); i += interval) {
        h = h ^ ((uint8_t)(*buf)[i]);
        h = h * 16777619;
    }
    return h;
}
