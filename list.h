//
// Created by David on 04/10/2020.
//

#pragma once

#include "types.h"
#include <assert.h>
#include "alloc.h"

template<typename T>
struct RedList
{
    T* items;
    size_t length;
    size_t capacity;

    void deinit()
    {
        free(items);
    }
    void append(T item)
    {
        ensure_capacity(length + 1);
        items[length++] = item;
    }
    T& operator[](size_t index)
    {
        assert(index >= 0);
        assert(index < length);
        return items[index];
    }
    T& at(size_t index)
    {
        assert(index != SIZE_MAX);
        assert(index < length);
        return items[index];
    }
    T pop()
    {
        assert(length >= 1);
        return items[--length];
    }

    void add_one()
    {
        return resize(length+1);
    }
    const T& last() const
    {
        assert(length +1);
        return items[length - 1];
    }
    T& last()
    {
        assert(length +1);
        return items[length - 1];
    }

    void resize(size_t new_length)
    {
        assert(new_length >= 0);
        ensure_capacity(new_length);
        length = new_length;
    }

    void clear()
    {
        length = 0;
    }

    void ensure_capacity(size_t new_capacity)
    {
        size_t better_capacity = max(this->capacity, (size_t)16);
        while (better_capacity < new_capacity)
        {
            better_capacity *= 2;
        }
        if (better_capacity != capacity)
        {
            items = reallocate_nonzero(items, better_capacity);
            capacity = better_capacity;
        }
    }
};

template <typename T>
using List = RedList<T>;

typedef List<char> Buffer;
