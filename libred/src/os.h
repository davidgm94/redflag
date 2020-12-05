//
// Created by david on 10/17/20.
//

#pragma once

#include "types.h"
#include <inttypes.h>
#include <string.h>


#if __linux__
#define RED_OS_LINUX
#elif _WIN32
#define RED_OS_WINDOWS
#else
#error
#endif

#ifdef RED_OS_LINUX
#define RED_PRI_usize "zu"
#define RED_PRI_s64 PRId64
#define RED_PRI_u64 PRIu64
#define RED_PRI_llu "llu"
#define OS_SEP "/"
#define RED_OS_SEP_CHAR '/'
#endif
#ifdef RED_OS_WINDOWS
#define RED_PRI_usize "zu"
#define RED_PRI_s64 PRId64
#define RED_PRI_u64 PRIu64
#define RED_PRI_llu "llu"
#define OS_SEP "/"
#define RED_OS_SEP_CHAR '/'
#endif

typedef union OS_SemaphoreInternal
{
    volatile void* win32_handle;
    volatile unsigned char posix_handle;
} volatile OS_Semaphore;

typedef enum TerminationID
{
    CLEAN,
    SIGNALED,
    STOPPED,
    UNKNOWN,
} TerminationID;

typedef struct Termination
{
    TerminationID type;
    s32 code;
} Termination;

typedef struct ExplicitTimer
{
    s64 start_time;
    char* text;
} ExplicitTimer;

ExplicitTimer os_timer_start(const char* text);
f64 os_timer_end(ExplicitTimer* et);

typedef enum FileLoadResult
{
    FILE_LOAD_RESULT_ERROR,
    FILE_LOAD_RESULT_SUCCESS,
} FileLoadResult;
typedef const char* os_arg;
typedef os_arg* os_arg_list;

#define strempty(str) str == 0 || *str == 0
#define strequal(a, b) strcmp(a, b) == 0

void red_panic(const char* file, size_t line, const char* function, const char* format, ...);
void os_abort();
void os_exit(s32 code);
void* allocate_chunk(size_t size);
void* reallocate_chunk(void* allocated_address, usize size);
void  mem_init(void);
void os_print_memory_usage(void);


#define RED_NOT_IMPLEMENTED { red_panic(__FILE__, __LINE__, __func__, "Not implemented"); __debugbreak(); os_exit(1); }
#define RED_UNREACHABLE { red_panic(__FILE__, __LINE__, __func__, "Unreachable"); __debugbreak(); os_exit(1); }
#define RED_PANIC(...) {  red_panic(__FILE__, __LINE__, __func__, __VA_ARGS__);  __debugbreak(); os_exit(1);}

#ifdef RED_DEBUG
#define redassert(_expr) if (!(_expr)) { RED_PANIC("Expression " #_expr " is false\n"); }
#else
#define redassert(_expr)
#endif

void print(const char* format, ...);

#define NEW(T, count) (T*)(allocate_chunk(count * sizeof(T)))
#define RENEW(T, old_ptr, count) (T*)(reallocate_chunk(old_ptr, count * sizeof(T)))


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
void os_init();
SB* os_get_cwd(void);
void* os_ask_virtual_memory_block(size_t block_bytes);
void* os_ask_virtual_memory_block_with_address(void* target_address, size_t block_bytes);
void* os_ask_heap_memory(size_t size);
size_t os_get_page_size(void);
void os_spawn_process(const char* exe, os_arg_list args, Termination* termination);
void os_abort(void);
void os_exit(s32 code);
void os_exit_with_message(const char* message, ...);
void os_print_recorded_times(f64 total_ms);
s64 os_performance_counter(void);
f64 os_compute_ms(s64 pc_start, s64 pc_end);
s32 os_load_dynamic_library(const char* dyn_lib_name);
void* os_load_procedure_from_dynamic_library(s32 dyn_lib_index, const char* proc_name);
StringBuffer* os_file_load(const char* name);



#define sb_assert_not_empty(sb) redassert(sb); redassert(sb->len)
#define sb_at(sb, i) (sb->ptr[i])
static inline size_t sb_len(StringBuffer* sb)
{
    redassert(sb);
    return sb->len - 1;
}

static inline char* sb_ptr(StringBuffer* sb)
{
    sb_assert_not_empty(sb);
    return sb->ptr;
}

static inline void sb_ensure_capacity(StringBuffer* sb, size_t new_capacity);
static inline void sb_resize(StringBuffer* sb, size_t new_length)
{
    new_length = new_length + 1;
    redassert(new_length != SIZE_MAX);
    sb_ensure_capacity(sb, new_length);
    sb->len = new_length;
    sb_at(sb, sb_len(sb)) = 0;
}

static inline void sb_ensure_capacity(StringBuffer* sb, size_t new_capacity)
{
    if (sb->cap >= new_capacity)
    {
        return;
    }

    usize better_capacity = sb->cap;
    do
    {
        better_capacity = better_capacity * 5 / 2 + 8;
    } while (better_capacity < new_capacity);

    sb->ptr = RENEW(char, sb->ptr, better_capacity);
    sb->cap = better_capacity;
}


static inline void sb_append_mem(SB* sb, const char* mem, s32 mem_len)
{
    sb_assert_not_empty(sb);
    redassert(mem_len >= 0);
    s32 old_len = sb_len(sb);
    sb_resize(sb, old_len + mem_len);
    memcpy(sb_ptr(sb) + old_len, mem, mem_len);
    sb_at(sb, sb_len(sb)) = 0;
}

static inline void sb_append_char(SB* sb, u8 c)
{
    sb_assert_not_empty(sb);
    sb_append_mem(sb, (const char*)&c, 1);
}

static inline void sb_append_s32(SB* sb, s32 n)
{
    sb_append_mem(sb, (char*)&n, sizeof(s32));
}

static inline void sb_append_str(SB* sb, const char* str)
{
    sb_assert_not_empty(sb);
    sb_append_mem(sb, str, strlen(str));
}

static inline StringBuffer* sb_alloc_fixed(size_t len)
{
    SB* sb = NEW(SB, 1);
    sb_resize(sb, len);
    return sb;
}
static inline StringBuffer* sb_alloc(void)
{
    return sb_alloc_fixed(0);
}

static inline void sb_memcpy(SB* sb, const char* ptr, size_t len)
{
    redassert(len != SIZE_MAX);
    sb_resize(sb, len);
    memcpy(sb_ptr(sb), ptr, len);
    sb_at(sb, sb_len(sb)) = 0;
}

static inline void sb_strcpy(SB* sb, const char* str)
{
    sb_memcpy(sb, str, strlen(str));
}

static inline void sb_clear(SB* sb)
{
    memset(sb->ptr, 0, sb->len);
    sb->len = 0;
}

static inline bool sb_cmp(SB* sb1, SB* sb2)
{
    s32 result = strcmp(sb1->ptr, sb2->ptr);
    return result == 0;
}

void prints(const char* msg);
void sb_vprintf(SB* sb, const char* format, va_list ap);
