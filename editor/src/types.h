#pragma once

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
#define static_assert(x, message) _Static_assert(x, message)
#endif // !__cplusplus

#define ZERO_INIT { 0 }

#define strempty(str) str == 0 || *str == 0
#define strequal(a, b) strcmp(a, b) == 0
#define array_length(_arr) ((sizeof(_arr))/ (sizeof(_arr[0])))
#define CASE_TO_STR(x) case(x): return #x
#define UNUSED_ELEM(x) x = x

//#define max(a, b) (((a) >= (b)) ? (a) : (b))
//#define min(a, b) (((a) <= (b)) ? (a) : (b))
#define char_to_int(c) (((s32)c) - 48)

typedef enum LogType
{
    LOG_TYPE_INFO,
    LOG_TYPE_WARN,
    LOG_TYPE_ERROR,
} LogType;

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


void print(const char* format, ...);
void logger(LogType log_type, const char *format, ...);

#define NEW(T, count) (T*)(allocate_chunk(count * sizeof(T)))
#define RENEW(T, old_ptr, count) (T*)(reallocate_chunk(old_ptr, count * sizeof(T)))

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

#define GEN_BUFFER_FUNCTIONS(p_type_prefix, buffer_name, t_type, elem_type)\
static inline u32 p_type_prefix##_len(t_type* buffer_name)\
{\
    return buffer_name->len;\
}\
static inline elem_type* p_type_prefix##_ptr(t_type* buffer_name)\
{\
    return buffer_name->ptr;\
}\
static inline void p_type_prefix##_deinit()\
{\
    RED_NOT_IMPLEMENTED;\
}\
\
static inline void p_type_prefix##_ensure_capacity(t_type* buffer_name, u32 new_capacity)\
{\
    if (buffer_name->cap >= new_capacity)\
        return;\
\
    u32 better_capacity = buffer_name->cap;\
    do {\
        better_capacity = better_capacity * 5 / 2 + 8;\
    } while (better_capacity < new_capacity);\
\
    buffer_name->ptr = RENEW(elem_type, buffer_name->ptr, better_capacity);\
    buffer_name->cap = better_capacity;\
}\
\
static inline void p_type_prefix##_resize(t_type* buffer_name, size_t new_length)\
{\
    redassert(new_length != SIZE_MAX);\
    p_type_prefix##_ensure_capacity(buffer_name, new_length);\
}\
\
static inline void p_type_prefix##_append(t_type* buffer_name, elem_type item)\
{\
    p_type_prefix##_ensure_capacity(buffer_name, buffer_name->len + 1);\
    buffer_name->ptr[buffer_name->len++] = item;\
}\
\
static inline void p_type_prefix##_append_assuming_capacity(t_type* buffer_name, elem_type item)\
{\
    buffer_name->ptr[buffer_name->len++] = item;\
}\
\
static inline elem_type p_type_prefix##_pop(t_type* buffer_name)\
{\
    redassert(buffer_name->len >= 1);\
    return buffer_name->ptr[--buffer_name->len];\
}\
\
static inline elem_type* p_type_prefix##_last(t_type* buffer_name)\
{\
    redassert(buffer_name->len >= 1);\
    return &buffer_name->ptr[buffer_name->len - 1];\
}\
\
static inline elem_type* p_type_prefix##_add_one(t_type* buffer_name)\
{\
    p_type_prefix##_resize(buffer_name, buffer_name->len + 1);\
    buffer_name->len++;\
    return p_type_prefix##_last(buffer_name);\
}\
\
static inline void p_type_prefix##_clear(t_type* buffer_name)\
{\
    buffer_name->len = 0;\
}

void* allocate_chunk(size_t size);
void* reallocate_chunk(void* allocated_address, usize size);
void  mem_init(void);

typedef struct StringBuffer
{
    char* ptr;
    u32 len; /* length */
    u32 cap; /* capacity */
} SB, StringBuffer;
