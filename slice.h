//
// Created by david on 10/17/20.
//

#pragma once
#include "types.h"
#include "heap.h"
#include <assert.h>

template<typename T>
struct Slice {
    T *ptr;
    size_t len;

    inline T &at(size_t i) {
        assert(i < len);
        return ptr[i];
    }

    inline Slice<T> slice(size_t start, size_t end) {
        assert(end <= len);
        assert(end >= start);
        return {
                ptr + start,
                end - start,
        };
    }

    inline Slice<T> sliceFrom(size_t start) {
        assert(start <= len);
        return {
                ptr + start,
                len - start,
        };
    }

    static inline Slice<T> alloc(size_t n) {
        return {heap::c_allocator.allocate_nonzero<T>(n), n};
    }
};

template<typename T>
static inline bool slice_eql(Slice<T> a, Slice<T> b) {
    if (a.len != b.len)
        return false;
    for (size_t i = 0; i < a.len; i += 1) {
        if (a.ptr[i] != b.ptr[i])
            return false;
    }
    return true;
}

template<typename T>
static inline bool slice_starts_with(Slice<T> haystack, Slice<T> needle) {
    if (needle.len > haystack.len)
        return false;
    return slice_eql(haystack.slice(0, needle.len), needle);
}

