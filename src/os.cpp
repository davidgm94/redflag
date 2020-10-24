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
#else
#endif


void os_init(void (*mem_init)(void))
{
#ifdef RED_OS_WINDOWS
    GetSystemInfo(&system_info);
    page_size = system_info.dwPageSize > system_info.dwAllocationGranularity ? system_info.dwPageSize : system_info.dwAllocationGranularity;
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
    print("The OS returned %zu characters for current directory\n", result);
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

void os_abort(void)
{
    abort();
}
