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
typedef _Bool bool;
#define true 1
#define false 0
#define nullptr null
#define static_assert(x, message) _Static_assert(x, message)
#endif // !__cplusplus

#define ZERO_INIT { 0 }

#define strempty(str) str == 0 || *str == 0
#define strequal(a, b) strcmp(a, b) == 0
#define array_length(_arr) ((sizeof(_arr))/ (sizeof(_arr[0])))
#define CASE_TO_STR(x) case(x): return #x
#define UNUSED_ELEM(x) x = x

#define max(a, b) (((a) >= (b)) ? (a) : (b))
#define min(a, b) (((a) <= (b)) ? (a) : (b))
#define char_to_int(c) (((s32)c) - 48)

void red_panic(const char* file, size_t line, const char* function, const char* format, ...);
void os_abort();
void os_exit(s32 code);

#define RED_NOT_IMPLEMENTED { red_panic(__FILE__, __LINE__, __func__, "Not implemented"); __debugbreak(); os_exit(1); }
#define RED_UNREACHABLE { red_panic(__FILE__, __LINE__, __func__, "Unreachable"); __debugbreak(); os_exit(1); }
#define RED_PANIC(...) {  red_panic(__FILE__, __LINE__, __func__, __VA_ARGS__);  __debugbreak(); os_exit(1);}

#define A_BYTE      (1ULL)
#define BYTE(x)     (x * A_BYTE)
#define KILOBYTE(x) (x * (BYTE(1024)))
#define MEGABYTE(x) (x * (KILOBYTE(1024)))
#define GIGABYTE(x) (x * (MEGABYTE(1024)))

#ifdef RED_DEBUG
#define redassert(_expr) if (!(_expr)) { RED_PANIC("Expression " #_expr " is false\n"); }
#else
#define redassert(_expr)
#endif

typedef struct StringBuffer
{
    char* ptr;
    u32 len; /* length */
    u32 cap; /* capacity */
} SB, StringBuffer;

