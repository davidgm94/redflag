//
// Created by david on 8/10/20.
//
#pragma once

#include "types.h"
#include <string.h>
#include <ctype.h>

static inline bool mem_eql_mem(const char* a, size_t a_len, const char* b, size_t b_len)
{
    if (a_len != b_len)
    {
        return false;
    }
    return memcmp(a, b, a_len) == 0;
}

static inline bool mem_eql_mem_ignore_case(const char* a, size_t a_len, const char* b, size_t b_len)
{
    if (a_len != b_len)
    {
        return false;
    }

    for (size_t i = 0; i < a_len; i++)
    {
        if (tolower(a[i]) != tolower(b[i]))
            return false;
    }
    return true;
}

static inline bool mem_eql_str(const char* mem, size_t mem_len, const char* str)
{
    return mem_eql_mem(mem, mem_len, str, strlen(str));
}

static inline bool str_eql_str(const char* a, const char* b)
{
    return mem_eql_mem(a, strlen(a), b, strlen(b));
}

static inline bool str_eql_str_ignore_case(const char* a, const char* b)
{
    return mem_eql_mem_ignore_case(a, strlen(a), b, strlen(b));
}

static inline bool is_power_of_2(u64 x)
{
    return x != 0 && ((x & (~x + 1)) == x);
}

static inline bool mem_ends_with_mem(const char* mem, size_t mem_len, const char* end, size_t end_len)
{
    if (mem_len < end_len)
    {
        return false;
    }
    return memcmp(mem + mem_len - end_len, end, end_len) == 0;
}

static inline bool mem_ends_with_str(const char* mem, size_t mem_len, const char* str)
{
    return mem_ends_with_mem(mem, mem_len, str, strlen(str));
}