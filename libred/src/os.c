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
#include <stdio.h>
#include <stdarg.h>

// Static cached structures 
#ifdef RED_OS_WINDOWS
static SYSTEM_INFO system_info;
static usize page_size;
static u16 logical_thread_count;
static LARGE_INTEGER pfreq;
static HINSTANCE loaded_dlls[1000];
static u16 dll_count = 0;

typedef struct TimeRecord
{
    f64 time_ms;
    SB* text;
} TimeRecord;

static struct TimeRecord records[100] = {0};
static u32 record_count = 0;

#else
#endif

ExplicitTimer os_timer_start(const char* text)
{
#ifdef RED_OS_WINDOWS
    ExplicitTimer et;
    QueryPerformanceCounter((LARGE_INTEGER*)&et.start_time);
    et.text = (char*)text;
    return et;
#else
#endif
}

f64 os_timer_end(ExplicitTimer* et)
{
#ifdef RED_OS_WINDOWS
    LARGE_INTEGER end;
    QueryPerformanceCounter(&end);
    f64 ms_time = (f64)(end.QuadPart - et->start_time) / (f64)(pfreq.QuadPart);
    SB* sb = sb_alloc();
    sb_strcpy(sb, et->text);
    redassert(record_count + 1 != array_length(records));
    records[record_count].time_ms = ms_time;
    records[record_count].text = sb;
    record_count++;

    return ms_time;
#else
#error
#endif
}
static inline void os_mem_init(void);

void os_init(void)
{
#ifdef RED_OS_WINDOWS
    GetSystemInfo(&system_info);
    page_size = system_info.dwPageSize > system_info.dwAllocationGranularity ? system_info.dwPageSize : system_info.dwAllocationGranularity;
    logical_thread_count = system_info.dwNumberOfProcessors;
    QueryPerformanceFrequency(&pfreq);
#else
#error
#endif
    os_mem_init();
}

SB* os_get_cwd(void)
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
    SB* sb = sb_alloc_fixed(result);
    sb_strcpy(sb, buffer);
    return sb;
#endif
}

void* os_ask_virtual_memory_block(size_t block_bytes)
{
    void* address = NULL;
#ifdef RED_OS_WINDOWS
    address = VirtualAlloc(NULL, block_bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
#error
#endif
    return address;
}

void* os_ask_virtual_memory_block_with_address(void* target_address, size_t block_bytes)
{
    void* address = NULL;
#ifdef RED_OS_WINDOWS
    address = VirtualAlloc(target_address, block_bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
#error
#endif
    return address;
}

void* os_ask_heap_memory(size_t size)
{
    void* address = NULL;
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

static void os_windows_create_command_line(SB* command_line, const char* exe, const char** args)
{
    sb_resize(command_line, 0);

    sb_append_char(command_line, '\"');
    sb_append_str(command_line, exe);
    sb_append_char(command_line, '\"');

    for (usize i = 0; i < command_line->len; i++)
    {
        sb_append_str(command_line, " \"");
        const char* arg = args[i];
        usize arg_len = strlen(arg);
        for (usize j = 0; j < arg_len; j++)
        {
            if (arg[j] == '\"')
            {
                RED_NOT_IMPLEMENTED;
            }
            sb_append_char(command_line, arg[j]);
        }
        sb_append_char(command_line, '\"');
    }
}

void os_spawn_process_windows(const char* exe, os_arg_list args, Termination* termination)
{
    SB* command_line = sb_alloc();
    os_windows_create_command_line(command_line, exe, args);

    PROCESS_INFORMATION process_info = {0};
    STARTUPINFOA startup_info = {0};
    startup_info.cb = sizeof(STARTUPINFO);

    SB* cwd = os_get_cwd();

    print("Sending command: %s\n", sb_ptr(command_line));
    BOOL success = CreateProcessA(exe, sb_ptr(command_line), NULL, NULL, TRUE, 0, NULL, sb_ptr(cwd), &startup_info, &process_info);
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

void os_spawn_process(const char* exe, os_arg_list args, Termination* termination)
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

void os_exit_with_message(const char* message, ...)
{
    va_list args;
    va_start(args, message);
    vfprintf(stdout, message, args);
    va_end(args);

    os_exit(1);
}

void os_print_recorded_times(f64 total_ms)
{
    //f64 rest_ms = total_ms;
    for (u32 i = 0; i < record_count; i++)
    {
        print("[%s]\t\t%02.02Lf%%\t\t%Lf ms.\t%.04Lf us.\t%.01Lf ns.\n", records[i].text->ptr, records[i].time_ms * 100 / total_ms, records[i].time_ms, records[i].time_ms * 1000, records[i].time_ms * 1000 * 1000);
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
    FARPROC fn_ptr = GetProcAddress(loaded_dlls[dyn_lib_index], proc_name);
    if (!fn_ptr)
    {
        print("Procedure %s not found in DLL\n", proc_name);
        exit(1);
    }

    return (void*)fn_ptr;
}
void sb_vprintf(SB* sb, const char* format, va_list ap)
{
    va_list ap2;
    va_copy(ap2, ap);

    int len1 = vsnprintf(NULL, 0, format, ap);
    redassert(len1 >= 0);

    size_t required_size = len1 + 1;
    sb_resize(sb, required_size);

    int len2 = vsnprintf(sb_ptr(sb), required_size, format, ap2);
    redassert(len2 == len1);

    va_end(ap2);
}

StringBuffer* os_file_load(const char *name)
{
    s32 rc;
    FILE *file = fopen(name, "rb");
    if (!file)
        return NULL;

    rc = fseek(file, 0, SEEK_END);
    if (rc) // not zero
    {
        return NULL;
    }
    size_t length = ftell(file);
    StringBuffer* file_buffer = sb_alloc_fixed(length);
    
    if (!file_buffer)
        return NULL;
    rc = fseek(file, 0, SEEK_SET);
    if (rc) // not zero
    {
        return NULL;
    }

    rc = fread(sb_ptr(file_buffer), 1, length, file);
    if (rc != (size_t) length)
    {
        return NULL;
    }

    rc = fclose(file);
    if (rc) // not zero
    {
        return NULL;
    }

    return file_buffer;
}

#define CACHE_LINE_SIZE BYTE(64)
#define MAX_ALLOCATED_BLOCK_COUNT 4
#define BLOCK_ALIGNMENT CACHE_LINE_SIZE
#define BLOCK_SIZE GIGABYTE(4)
#define DEFAULT_ALIGNMENT 16

struct MemoryBlock
{
    void* address;
    void* aligned_address;
    void* available_address;
};

typedef struct Allocation
{
    u32 alignment;
    u32 size;
} Allocation;

typedef struct PageAllocator
{
    void* available_address;
    usize allocation_count;
    usize allocated_block_count;
    void* blob;
    usize page_size;
} PageAllocator;

static struct PageAllocator m_page_allocator;

typedef enum AllocationResult
{
    ALLOCATION_RESULT_ERROR,
    ALLOCATION_RESULT_SUCCESS,
} AllocationResult;


// @NOT_USED
//static inline usize round_up_to_next_page(usize size, usize page_size)
//{
//    usize remainder = size % page_size;
//    usize to_add = page_size - remainder;
//    return size + to_add;
//}

static inline void* align_address(void* address, usize align)
{
    const size_t mask = align - 1;
    uptr p = (uptr)address;
    redassert((align & mask) == 0);

    return (void*)((p + mask) & ~mask);
}

static inline void fill_with_allocation_garbage(void* start, void* end)
{
    u8* it = (u8*)start;
    while (it != end)
    {
        *it++ = 0xff;
    }
}

static inline Allocation* find_allocation_metadata(void* visible_address)
{
    u8* it = (u8*)((uptr)visible_address - sizeof(Allocation));
    u32* alignment_ptr = (u32*)it;
    redassert(*alignment_ptr == 16);
    redassert(it < (u8*)visible_address);
    return (Allocation*)it;
}

//static inline void buffer_zero_check(void* address, size_t size)
//{
//    if (address)
//    {
//        u8* it = (u8*)address;
//        for (usize i = 0; i < size; i++)
//        {
//            if (it[i] != 0)
//            {
//                redassert(it[i] == 0);
//            }
//        }
//    }
//}

static inline void allocate_new_block()
{
    void* target_address = NULL;
    if (m_page_allocator.allocation_count > 0)
    {
        void* original_address = m_page_allocator.blob;
        target_address = (void*)((uptr)original_address * (m_page_allocator.allocation_count + 1));
    }
    void* address = os_ask_virtual_memory_block_with_address(target_address, BLOCK_SIZE);
    redassert(address != NULL);
    if (target_address)
    {
        redassert(address == target_address);
    }

#if RED_BUFFER_MEM_CHECK
    buffer_zero_check(address, block_size);
#endif

    if (address)
    {
        void* aligned_address = align_address(address, BLOCK_ALIGNMENT);
        redassert(aligned_address == address);
        usize lost_memory = (uptr)aligned_address - (uptr)address;
        redassert(lost_memory == 0);
        usize aligned_size = BLOCK_SIZE - lost_memory;
        redassert(aligned_size == BLOCK_SIZE);
        if (target_address == NULL)
        {
            m_page_allocator.blob = address;
            m_page_allocator.available_address = address;
        }
        else
        {
            m_page_allocator.available_address = target_address;
        }
        m_page_allocator.allocated_block_count++;
    }
    else
    {
        os_exit_with_message("Block allocation failed!");
    }
}

static inline uptr top_address(void)
{
    return (uptr)m_page_allocator.blob + (m_page_allocator.allocated_block_count * BLOCK_SIZE);
}

void* allocate_chunk(usize size)
{
#if RED_BUFFER_MEM_CHECK
    buffer_zero_check(m_page_allocator.available_address, (uptr)m_page_allocator.blob +  block_size - (uptr)m_page_allocator.available_address);
#endif
    redassert(m_page_allocator.allocated_block_count <= MAX_ALLOCATED_BLOCK_COUNT);
    // TODO:
    usize max_required_size = size + DEFAULT_ALIGNMENT;
    redassert(sizeof(Allocation) < DEFAULT_ALIGNMENT);

    bool first_allocation = m_page_allocator.allocated_block_count == 0;
    bool can_allocate_in_current_block = !first_allocation;

    if (!first_allocation)
    {
        uptr available_address = (uptr)m_page_allocator.available_address;
        usize free_space = top_address() - available_address;
        bool no_need_to_allocate_another_block = max_required_size < free_space;
        can_allocate_in_current_block = can_allocate_in_current_block && no_need_to_allocate_another_block;
    }

    if (!can_allocate_in_current_block)
    {
        allocate_new_block();
    }

    // This is the address with the allocation metadata
    void* address = m_page_allocator.available_address;
    void* aligned_address = align_address((void*)((uptr)address + sizeof(Allocation)), DEFAULT_ALIGNMENT);

    fill_with_allocation_garbage(address, aligned_address);

    usize pointer_displacement = (uptr)aligned_address - (uptr)address + size;
    uptr new_available_address_number = (uptr)address + pointer_displacement;
    void* new_available_address = (void*)new_available_address_number;
    redassert(new_available_address_number < top_address());
    Allocation* allocation = (Allocation*)((uptr)aligned_address - sizeof(Allocation));
    redassert(sizeof(Allocation) == (sizeof(u32) * 2));
    allocation->alignment = DEFAULT_ALIGNMENT;
    allocation->size = pointer_displacement;
    redassert(allocation->size != 0);
    m_page_allocator.available_address = new_available_address;
    m_page_allocator.allocation_count++;

#if RED_BUFFER_MEM_CHECK
    buffer_zero_check(aligned_address, size);
#endif
#if RED_ALLOCATION_VERBOSE
    print("[ALLOCATOR] (#%zu) Allocating %zu bytes at address 0x%p. Total allocated: %zu\n", m_page_allocator.allocation_count, allocation->size, address, (usize)((uptr)m_page_allocator.available_address - (uptr)m_page_allocator.blob));
#endif
    redassert(new_available_address != address);

#if RED_BUFFER_MEM_CHECK
    buffer_zero_check(m_page_allocator.available_address, (uptr)m_page_allocator.blob +  block_size - (uptr)m_page_allocator.available_address);
#endif
    return (void*)aligned_address;
}

void* reallocate_chunk(void* allocated_address, usize size)
{
    redassert(size > 0);
    redassert(size < UINT32_MAX);
    if (!allocated_address)
    {
        return allocate_chunk(size);
    }
    
    Allocation* allocation_metadata = find_allocation_metadata(allocated_address);
    usize difference = (uptr)allocated_address - (uptr)allocation_metadata;
    usize real_size = allocation_metadata->size - difference;
    redassert(real_size < size);
    void* new_address = allocate_chunk(size);
    memcpy(new_address, allocated_address, real_size);

    return new_address;
}

void print(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
}

void prints(const char* msg)
{
    (void)puts(msg);
}

void red_panic(const char* file, size_t line, const char* function, const char* format, ...)
{
    char buffer [1024];
    char buffer2[1024];

    //memset(buffer, 0x00, sizeof(buffer));

    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);

    sprintf(buffer2, "Panic at %s:%zu: %s -> %s\n", file, line, function, buffer);
    MessageBoxA(GetActiveWindow(), buffer2, "PANIC", MB_ABORTRETRYIGNORE);
}

void os_print_memory_usage(void)
{
    redassert(m_page_allocator.allocated_block_count == 1);
    u64 mem_usage = (usize)((uptr)m_page_allocator.available_address - (uptr)m_page_allocator.blob);
    u64 block_size = BLOCK_SIZE;
    u64 alloc_count = m_page_allocator.allocation_count;
    print("\nMemory usage: %llu bytes. Available: %llu bytes. Relative usage: %02.02f%%. Total allocations: %llu\n", mem_usage, block_size, ((f64)mem_usage / (f64)block_size) * 100.0f, alloc_count);
}

static inline void os_mem_init(void)
{
m_page_allocator.page_size = os_get_page_size();
}
