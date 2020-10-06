/*
 * Copyright (c) 2015 Andrew Kelley
 *
 * This file is part of zig, which is MIT licensed.
 * See http://opensource.org/licenses/MIT
 */

#ifndef ZIG_BUFFER_HPP
#define ZIG_BUFFER_HPP

#include "list.hpp"

#include <stdint.h>
#include <ctype.h>
#include <stdarg.h>

#define BUF_INIT {{0}}

typedef ZigList<char> Buffer;

Buffer *buf_sprintf(const char *format, ...)
    ATTRIBUTE_PRINTF(1, 2);
Buffer *buf_vprintf(const char *format, va_list ap);

static inline size_t buf_len(Buffer *buf) {
    assert(buf);
    assert(buf->list.length);
    return buf->list.length - 1;
}

static inline char *buf_ptr(Buffer *buf) {
    assert(buf);
    assert(buf->list.length);
    return buf->list.items;
}

static inline const char *buf_ptr(const Buffer *buf) {
    assert(buf);
    assert(buf->list.length);
    return buf->list.items;
}

static inline void buf_resize(Buffer *buf, size_t new_len) {
    buf->list.resize(new_len + 1);
    buf->list.at(buf_len(buf)) = 0;
}

static inline Buffer *buf_alloc_fixed(size_t size) {
    Buffer *buf = heap::c_allocator.create<Buffer>();
    buf_resize(buf, size);
    return buf;
}

static inline Buffer *buf_alloc(void) {
    return buf_alloc_fixed(0);
}

static inline void buf_deinit(Buffer *buf) {
    buf->list.deinit();
}

static inline void buf_destroy(Buffer *buf) {
    buf_deinit(buf);
    heap::c_allocator.destroy(buf);
}

static inline void buf_init_from_mem(Buffer *buf, const char *ptr, size_t len) {
    assert(len != SIZE_MAX);
    buf->list.resize(len + 1);
    memcpy(buf_ptr(buf), ptr, len);
    buf->list.at(buf_len(buf)) = 0;
}

static inline void buf_init_from_str(Buffer *buf, const char *str) {
    buf_init_from_mem(buf, str, strlen(str));
}

static inline void buf_init_from_buf(Buffer *buf, Buffer *other) {
    buf_init_from_mem(buf, buf_ptr(other), buf_len(other));
}

static inline Buffer *buf_create_from_mem(const char *ptr, size_t len) {
    assert(len != SIZE_MAX);
    Buffer *buf = heap::c_allocator.create<Buffer>();
    buf_init_from_mem(buf, ptr, len);
    return buf;
}

static inline Buffer *buf_create_from_slice(Slice<uint8_t> slice) {
    return buf_create_from_mem((const char *)slice.ptr, slice.len);
}

static inline Buffer *buf_create_from_str(const char *str) {
    return buf_create_from_mem(str, strlen(str));
}

static inline Buffer *buf_create_from_buf(Buffer *buf) {
    return buf_create_from_mem(buf_ptr(buf), buf_len(buf));
}

static inline Buffer *buf_slice(Buffer *in_buf, size_t start, size_t end) {
    assert(in_buf->list.length);
    assert(start != SIZE_MAX);
    assert(end != SIZE_MAX);
    assert(start < buf_len(in_buf));
    assert(end <= buf_len(in_buf));
    Buffer *out_buf = heap::c_allocator.create<Buffer>();
    out_buf->list.resize(end - start + 1);
    memcpy(buf_ptr(out_buf), buf_ptr(in_buf) + start, end - start);
    out_buf->list.at(buf_len(out_buf)) = 0;
    return out_buf;
}

static inline void buf_append_mem(Buffer *buf, const char *mem, size_t mem_len) {
    assert(buf->list.length);
    assert(mem_len != SIZE_MAX);
    size_t old_len = buf_len(buf);
    buf_resize(buf, old_len + mem_len);
    memcpy(buf_ptr(buf) + old_len, mem, mem_len);
    buf->list.at(buf_len(buf)) = 0;
}

static inline void buf_append_str(Buffer *buf, const char *str) {
    assert(buf->list.length);
    buf_append_mem(buf, str, strlen(str));
}

static inline void buf_append_buf(Buffer *buf, Buffer *append_buf) {
    assert(buf->list.length);
    buf_append_mem(buf, buf_ptr(append_buf), buf_len(append_buf));
}

static inline void buf_append_char(Buffer *buf, uint8_t c) {
    assert(buf->list.length);
    buf_append_mem(buf, (const char *)&c, 1);
}

void buf_appendf(Buffer *buf, const char *format, ...)
    ATTRIBUTE_PRINTF(2, 3);

static inline bool buf_eql_mem(Buffer *buf, const char *mem, size_t mem_len) {
    assert(buf->list.length);
    return mem_eql_mem(buf_ptr(buf), buf_len(buf), mem, mem_len);
}

static inline bool buf_eql_mem_ignore_case(Buffer *buf, const char *mem, size_t mem_len) {
    assert(buf->list.length);
    return mem_eql_mem_ignore_case(buf_ptr(buf), buf_len(buf), mem, mem_len);
}

static inline bool buf_eql_str(Buffer *buf, const char *str) {
    assert(buf->list.length);
    return buf_eql_mem(buf, str, strlen(str));
}

static inline bool buf_eql_str_ignore_case(Buffer *buf, const char *str) {
    assert(buf->list.length);
    return buf_eql_mem_ignore_case(buf, str, strlen(str));
}

static inline bool buf_starts_with_mem(Buffer *buf, const char *mem, size_t mem_len) {
    if (buf_len(buf) < mem_len) {
        return false;
    }
    return memcmp(buf_ptr(buf), mem, mem_len) == 0;
}

static inline bool buf_starts_with_buf(Buffer *buf, Buffer *sub) {
    return buf_starts_with_mem(buf, buf_ptr(sub), buf_len(sub));
}

static inline bool buf_starts_with_str(Buffer *buf, const char *str) {
    return buf_starts_with_mem(buf, str, strlen(str));
}

static inline bool buf_ends_with_mem(Buffer *buf, const char *mem, size_t mem_len) {
    return mem_ends_with_mem(buf_ptr(buf), buf_len(buf), mem, mem_len);
}

static inline bool buf_ends_with_str(Buffer *buf, const char *str) {
    return buf_ends_with_mem(buf, str, strlen(str));
}

bool buf_eql_buf(Buffer *buf, Buffer *other);
uint32_t buf_hash(Buffer *buf);

static inline void buf_upcase(Buffer *buf) {
    for (size_t i = 0; i < buf_len(buf); i += 1) {
        buf_ptr(buf)[i] = (char)toupper(buf_ptr(buf)[i]);
    }
}

static inline Slice<uint8_t> buf_to_slice(Buffer *buf) {
    return Slice<uint8_t>{reinterpret_cast<uint8_t*>(buf_ptr(buf)), buf_len(buf)};
}

static inline void buf_replace(Buffer* buf, char from, char to) {
    const size_t count = buf_len(buf);
    char* ptr = buf_ptr(buf);
    for (size_t i = 0; i < count; ++i) {
        char& l = ptr[i];
        if (l == from)
            l = to;
    }
}

#endif
