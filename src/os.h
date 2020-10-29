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

enum TerminationID
{
    CLEAN,
    SIGNALED,
    STOPPED,
    UNKNOWN,
};

struct Termination
{
    TerminationID type;
    s32 code;
};

struct ExplicitTimer
{
    s64 start_time;
    char* text;
    ExplicitTimer(const char* text);
    void end();
};

struct ScopeTimer
{
#ifdef RED_OS_WINDOWS
    ExplicitTimer time_period;

    ScopeTimer(const char* text)
        : time_period(text)
    { }

    ~ScopeTimer()
    {
        time_period.end();
    }
#else
#error
#endif
};

void os_init(void(*mem_init)(void));
Error os_get_cwd(Buffer* out_cwd);
void* os_ask_virtual_memory_block(size_t block_bytes);
void* os_ask_virtual_memory_block_with_address(void* target_address, size_t block_bytes);
void* os_ask_heap_memory(size_t size);
size_t os_get_page_size(void);
void os_spawn_process(const char* exe, List<const char*> args, Termination* termination);
void os_abort(void);
void os_exit(s32 code);
void os_print_recorded_times(f64 total_ms);
s64 os_performance_counter(void);
f64 os_compute_ms(s64 pc_start, s64 pc_end);
s32 os_load_dynamic_library(const char* dyn_lib_name);
void* os_load_procedure_from_dynamic_library(s32 dyn_lib_index, const char* proc_name);
