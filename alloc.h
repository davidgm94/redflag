//
// Created by David on 04/10/2020.
//

#ifndef REDFLAG_ALLOC_H
#define REDFLAG_ALLOC_H
#include "types.h"
#include <malloc.h>

template<typename T>
static inline T* ealloc(size_t count)
{
    return reinterpret_cast<T*>(malloc(count * sizeof(T)));
}
template <typename T>
static inline T* salloc(size_t size)
{
    return reinterpret_cast<T*>(malloc(size));
}

template <typename T>
static inline T* reallocate_nonzero(T* old, size_t new_count)
{
    T* ptr = reinterpret_cast<T*>(realloc(old, new_count * sizeof(T)));
    if (!ptr)
        PANIC();
    return ptr;
}

#endif //REDFLAG_ALLOC_H
