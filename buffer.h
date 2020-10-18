//
// Created by David on 04/10/2020.
//

#ifndef REDFLAG_BUFFER_H
#define REDFLAG_BUFFER_H

#include "compiler_types.h"
#include <string.h>


Buffer *buf_sprintf(const char *format, ...)
__attribute__ ((format (printf, 1, 2)));

static inline int buf_len(Buffer *buf) {
    assert(buf->length);
    return buf->length - 1;
}

static inline char *buf_ptr(Buffer *buf) {
    assert(buf->length);
    return buf->items;
}

static inline void buf_resize(Buffer *buf, int new_len) {
    buf->resize(new_len + 1);
    buf->items[buf_len(buf)] = 0;
}

static inline Buffer *buf_alloc(void) {
    Buffer *buf = new_elements(Buffer, 1);
    buf_resize(buf, 0);
    return buf;
}

static inline Buffer *buf_alloc_fixed(int size) {
    Buffer *buf = new_elements(Buffer, 1);
    memset(buf, 0, sizeof(Buffer));
    buf_resize(buf, size);
    return buf;
}

static inline void buf_deinit(Buffer *buf) {
    buf->deinit();
}

static inline void buf_init_from_mem(Buffer *buf, const char *ptr, int len) {
    memset(buf, 0, sizeof(Buffer));
    buf->resize(len + 1);
    memcpy(buf_ptr(buf), ptr, len);
    buf->at(buf_len(buf)) = 0;
}

static inline void buf_init_from_str(Buffer *buf, const char *str) {
    buf_init_from_mem(buf, str, strlen(str));
}

static inline void buf_init_from_buf(Buffer *buf, Buffer *other) {
    buf_init_from_mem(buf, buf_ptr(other), buf_len(other));
}

static inline Buffer *buf_create_from_mem(const char *ptr, int len) {
    Buffer *buf = new_elements(Buffer, 1);
    buf_init_from_mem(buf, ptr, len);
    return buf;
}

static inline Buffer *buf_create_from_str(const char *str) {
    return buf_create_from_mem(str, strlen(str));
}

static inline Buffer *buf_slice(Buffer *in_buf, int start, int end) {
    assert(in_buf->length);
    assert(start >= 0);
    assert(end >= 0);
    assert(start < buf_len(in_buf));
    assert(end <= buf_len(in_buf));
    Buffer *out_buf = new_elements(Buffer, 1);
    out_buf->resize(end - start + 1);
    memcpy(buf_ptr(out_buf), buf_ptr(in_buf) + start, end - start);
    (*out_buf)[buf_len(out_buf)] = 0;
    return out_buf;
}

static inline void buf_append_mem(Buffer *buf, const char *mem, int mem_len) {
    assert(buf->length);
    assert(mem_len >= 0);
    int old_len = buf_len(buf);
    buf_resize(buf, old_len + mem_len);
    memcpy(buf_ptr(buf) + old_len, mem, mem_len);
    (*buf)[buf_len(buf)] = 0;
}

static inline void buf_append_str(Buffer *buf, const char *str) {
    assert(buf->length);
    buf_append_mem(buf, str, strlen(str));
}

static inline void buf_append_buf(Buffer *buf, Buffer *append_buf) {
    assert(buf->length);
    buf_append_mem(buf, buf_ptr(append_buf), buf_len(append_buf));
}

static inline void buf_append_char(Buffer *buf, uint8_t c) {
    assert(buf->length);
    buf_append_mem(buf, (const char *)&c, 1);
}

void buf_appendf(Buffer *buf, const char *format, ...)
__attribute__ ((format (printf, 2, 3)));

static inline bool buf_eql_mem(Buffer *buf, const char *mem, int mem_len) {
    assert(buf->length);
    if (buf_len(buf) != mem_len)
        return false;
    return memcmp(buf_ptr(buf), mem, mem_len) == 0;
}

static inline bool buf_eql_str(Buffer *buf, const char *str) {
    assert(buf->length);
    return buf_eql_mem(buf, str, strlen(str));
}

static inline bool buf_starts_with_mem(Buffer *buf, const char *mem, size_t mem_len) {
    if (buf_len(buf) < mem_len) {
        return false;
    }
    return memcmp(buf_ptr(buf), mem, mem_len) == 0;
}

bool buf_eql_buf(Buffer *buf, Buffer *other);
uint32_t buf_hash(Buffer *buf);

Buffer* buf_vprintf(const char *format, va_list ap);

#endif //REDFLAG_BUFFER_H
