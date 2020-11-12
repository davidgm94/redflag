//
// Created by david on 10/17/20.
//

#pragma once
#include "compiler_types.h"
#include <inttypes.h>

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
void os_timer_end(ExplicitTimer* et);

typedef enum FileLoadResult
{
    FILE_LOAD_RESULT_ERROR,
    FILE_LOAD_RESULT_SUCCESS,
} FileLoadResult;
typedef const char* os_arg;
typedef os_arg* os_arg_list;

void os_init(void(*mem_init)(void));
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
    return strcmp(sb1->ptr, sb2->ptr) == 0;
}

void prints(const char* msg);
void sb_vprintf(SB* sb, const char* format, va_list ap);
