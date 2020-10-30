//
// Created by david on 10/17/20.
//

#include "os.h"

#ifdef RED_OS_LINUX
#define RED_OS_POSIX
#include <unistd.h>
#include <linux/limits.h>
#elif defined RED_OS_WINDOWS
#include <Windows.h>
#endif

#include <errno.h>
#include <stdlib.h>

// Static cached structures 
#ifdef RED_OS_WINDOWS
static SYSTEM_INFO system_info;
static size_t page_size;
static LARGE_INTEGER pfreq;
static HINSTANCE loaded_dlls[1000];
static u16 dll_count = 0;

struct TimeRecord
{
    f64 time_ms;
    Buffer* text;
};

static struct TimeRecord records[100] = {};
static u32 record_count = 0;

#else
#endif

ExplicitTimer::ExplicitTimer(const char* text)
{
#ifdef RED_OS_WINDOWS
    QueryPerformanceCounter((LARGE_INTEGER*)&this->start_time);
    this->text = (char*)text;
#else
#endif
}

void ExplicitTimer::end()
{
#ifdef RED_OS_WINDOWS
    LARGE_INTEGER end;
    QueryPerformanceCounter(&end);
    f64 ms_time = f64(end.QuadPart - start_time) / f64(pfreq.QuadPart);
    Buffer* bf = buf_alloc();
    buf_init_from_str(bf, this->text);
    redassert(record_count + 1 != array_length(records));
    records[record_count++] = { ms_time, bf };
#else
#error
#endif
}

void os_init(void (*mem_init)(void))
{
#ifdef RED_OS_WINDOWS
    GetSystemInfo(&system_info);
    page_size = system_info.dwPageSize > system_info.dwAllocationGranularity ? system_info.dwPageSize : system_info.dwAllocationGranularity;
    QueryPerformanceFrequency(&pfreq);
#else
#error
#endif
    mem_init();
}

Error os_get_cwd(Buffer*out_cwd)
{
#ifdef RED_OS_POSIX
    char buffer[PATH_MAX];
    char* result = getcwd(buffer, PATH_MAX);
    if (!result)
    {
        RED_PANIC("Unable to get cwd: %s", strerror(errno));
    }
    buf_init_from_str(out_cwd, result);
    return ERROR_NONE;
#else
    char buffer[MAX_PATH];
    DWORD result = GetCurrentDirectoryA(MAX_PATH, buffer);
    if (result == 0)
    {
        RED_PANIC("Unable to get cwd", strerror(errno));
    }
    // print("The OS returned %zu characters for current directory\n", result);
    buf_init_from_str(out_cwd, buffer);

    return ERROR_NONE;
#endif
}

void* os_ask_virtual_memory_block(size_t block_bytes)
{
    void* address = nullptr;
#ifdef RED_OS_WINDOWS
    address = VirtualAlloc(nullptr, block_bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
#error
#endif
    return address;
}

void* os_ask_virtual_memory_block_with_address(void* target_address, size_t block_bytes)
{
    void* address = nullptr;
#ifdef RED_OS_WINDOWS
    address = VirtualAlloc(target_address, block_bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
#error
#endif
    return address;
}

void* os_ask_heap_memory(size_t size)
{
    void* address = nullptr;
#ifdef RED_OS_WINDOWS
    address = malloc(size);
#endif
    return address;
}

size_t os_get_page_size(void)
{
#ifdef RED_OS_WINDOWS
    return page_size;
#endif
}

static void os_windows_create_command_line(Buffer* command_line, const char* exe, List<const char*> args)
{
    buf_resize(command_line, 0);

    buf_append_char(command_line, '\"');
    buf_append_str(command_line, exe);
    buf_append_char(command_line, '\"');

    for (usize i = 0; i < args.length; i++)
    {
        buf_append_str(command_line, " \"");
        const char* arg = args.at(i);
        usize arg_len = strlen(arg);
        for (usize j = 0; j < arg_len; j++)
        {
            if (arg[j] == '\"')
            {
                RED_NOT_IMPLEMENTED;
            }
            buf_append_char(command_line, arg[j]);
        }
        buf_append_char(command_line, '\"');
    }
}

void os_spawn_process_windows(const char* exe, List<const char*> args, Termination* termination)
{
    Buffer command_line = {};
    os_windows_create_command_line(&command_line, exe, args);

    PROCESS_INFORMATION process_info = {};
    STARTUPINFOA startup_info = {};
    startup_info.cb = sizeof(STARTUPINFO);

    Buffer cwd_buffer = {};
    os_get_cwd(&cwd_buffer);

    BOOL success = CreateProcessA(exe, buf_ptr(&command_line), nullptr, nullptr, TRUE, 0, nullptr, buf_ptr(&cwd_buffer), &startup_info, &process_info);
    if (!success)
    {
        RED_PANIC("wtf");
    }

    WaitForSingleObject(process_info.hProcess, INFINITE);

    DWORD exit_code;
    if (!GetExitCodeProcess(process_info.hProcess, &exit_code))
    {
        RED_PANIC("panic");
    }

    termination->type = CLEAN;
    termination->code = exit_code;
}

void os_spawn_process(const char* exe, List<const char*> args, Termination* termination)
{
#ifdef RED_OS_WINDOWS
    os_spawn_process_windows(exe, args, termination);
#else
#error
#endif
}

void os_exit(s32 code)
{
    exit(code);
}

void os_print_recorded_times(f64 total_ms)
{
    //f64 rest_ms = total_ms;
    for (u32 i = 0; i < record_count; i++)
    {
        print("[%s]\t\t%02.02Lf%%\t\t%Lf ms.\t%.04Lf us.\t%.01Lf ns.\n", buf_ptr(records[i].text), records[i].time_ms * 100 / total_ms, records[i].time_ms, records[i].time_ms * 1000, records[i].time_ms * 1000 * 1000);
        //if (i != 2)
        //{
        //    rest_ms -= records[i].time_ms;
        //}
    }
    //print("[Rest]\t\t%02.02Lf%%\t\t%Lf ms.\t%.04Lf us.\t%.01Lf ns.\n", rest_ms * 100 / total_ms, rest_ms, rest_ms * 1000, rest_ms * 1000 * 1000);
    print("[Total]\t\t%02.02Lf%%\t\t%Lf ms.\t%.04Lf us.\t%.01Lf ns.\n", 100.0, total_ms, total_ms * 1000, total_ms * 1000 * 1000);
}

void os_abort(void)
{
    abort();
}

s64 os_performance_counter(void)
{
    s64 pc;
    QueryPerformanceCounter((LARGE_INTEGER*)&pc);
    return pc;
}

f64 os_compute_ms(s64 pc_start, s64 pc_end)
{
    return (f64)(pc_end - pc_start) / (f64)pfreq.QuadPart;
}

s32 os_load_dynamic_library(const char* dyn_lib_name)
{
#ifdef RED_OS_WINDOWS
    HINSTANCE dll_instance = LoadLibraryA(dyn_lib_name);
    if (!dll_instance)
    {
        print("DLL %s not found\n", dyn_lib_name);
        exit(1);
    }

    s32 id = dll_count++;
    loaded_dlls[id] = dll_instance;
    return id;
#else
#endif
}

void* os_load_procedure_from_dynamic_library(s32 dyn_lib_index, const char* proc_name)
{
    auto fn_ptr = GetProcAddress(loaded_dlls[dyn_lib_index], proc_name);
    if (!fn_ptr)
    {
        print("Procedure %s not found in DLL\n", proc_name);
        exit(1);
    }

    return (void*)fn_ptr;
}
