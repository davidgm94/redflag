#pragma once
#include "config.h"

#include <stdint.h>
#include <stddef.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uintptr_t uptr;
typedef size_t usize;
typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef struct { u16 v; } float16_t;
typedef float16_t f16;
typedef float f32;
typedef double f64;
typedef struct { u64 v[2]; } float128_t;
typedef float128_t f128;
#define null NULL
#ifndef __cplusplus
#include <stdbool.h>
#define nullptr null
#endif // !__cplusplus

#define ZERO_INIT { 0 }

#define array_length(_arr) ((sizeof(_arr))/ (sizeof(_arr[0])))
#define CASE_TO_STR(x) case(x): return #x
#define UNUSED_ELEM(x) x = x

//#define max(a, b) (((a) >= (b)) ? (a) : (b))
//#define min(a, b) (((a) <= (b)) ? (a) : (b))
#define char_to_int(c) (((s32)c) - 48)

#define A_BYTE      (1ULL)
#define BYTE(x)     (x * A_BYTE)
#define KILOBYTE(x) (x * (BYTE(1024)))
#define MEGABYTE(x) (x * (KILOBYTE(1024)))
#define GIGABYTE(x) (x * (MEGABYTE(1024)))

typedef struct SB
{
    char* ptr;
    u32 len; /* length */
    u32 cap; /* capacity */
} SB, StringBuffer;

#define GEN_BUFFER_STRUCT_PTR(type_name, type) \
    typedef struct type_name##Buffer \
    {\
        struct type* ptr;\
        u32 len;\
        u32 cap;\
    } type_name##Buffer;

#define GEN_BUFFER_STRUCT(type)\
    typedef struct type##Buffer \
    {\
        type* ptr;\
        u32 len;\
        u32 cap;\
    } type##Buffer;

#define GEN_BUFFER_STRUCT_PTR_NO_STRUCT(name, type) \
    typedef struct name##Buffer \
    {                    \
        type* ptr;                 \
        u32 len;                  \
        u32 cap;                  \
    } name##Buffer;
#define BIT_FIELD_4(first, second, third, fourth) \
    u8 first:1;\
    u8 second:1;\
    u8 third:1;\
    u8 fourth:1
