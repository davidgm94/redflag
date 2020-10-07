#pragma once
#include <stdint.h>
#include <assert.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef float f32;
typedef double f64;

#define strempty(str) str == 0 || *str == 0
#define strequal(a, b) strcmp(a, b) == 0
#define COUNT_OF(x) ((sizeof(x))/(sizeof((x[0]))))
#define CASE_TO_STR(x) case(x): return #x
#define UNUSED_ELEM(x) x = x


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

