#pragma once
#include <stdint.h>
#include <stddef.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
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

#define strempty(str) str == 0 || *str == 0
#define strequal(a, b) strcmp(a, b) == 0
template <typename T, size_t n>
inline constexpr size_t array_length(const T (&)[n])
{
    return n;
}
#define CASE_TO_STR(x) case(x): return #x
#define UNUSED_ELEM(x) x = x


template <typename T>
static inline T max(T a, T b)
{
    return (a >= b) ? a : b;
}
//template <typename T>
//static inline T max(T& a, T& b)
//{
//    return (a >= b) ? a : b;
//}
//template <typename T>
//static inline T max(const T& a, const T& b)
//{
//    return (a >= b) ? a : b;
//}
template <typename T>
static inline T min(T a, T b)
{
    return (a <= b) ? a : b;
}

static inline s32 char_to_int(char c)
{
    return (s32)c - 48;
}
enum LogType
{
    LOG_TYPE_INFO,
    LOG_TYPE_WARN,
    LOG_TYPE_ERROR,
};
struct TypeInfo
{
    size_t size;
    size_t alignment;

    template<typename T>
    static constexpr TypeInfo make()
    {
        return {sizeof(T), alignof(T)};
    }
};
static constexpr size_t max_filename_length = 512;
struct Allocation
{
    char file[max_filename_length];
    char function[max_filename_length];
    char type[max_filename_length];
    TypeInfo type_info;
    void* address;
    size_t line;
};
static constexpr size_t MAX_ALLOCS = 1024 * 100;
struct Allocator
{
    Allocation allocations[MAX_ALLOCS];
    size_t allocation_count;
};
static Allocator allocator = {};

void red_panic(const char* file, int line, const char* function, const char* format, ...);
#define RED_NOT_IMPLEMENTED red_panic(__FILE__, __LINE__, __func__, "Not implemented")
#define RED_UNREACHABLE red_panic(__FILE__, __LINE__, __func__, "Unreachable")
#define RED_PANIC(...) red_panic(__FILE__, __LINE__, __func__, __VA_ARGS__)
