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

void os_init(void(*mem_init)(void));
Error os_get_cwd(Buffer* out_cwd);
void* os_ask_virtual_memory_block(size_t block_bytes);
void* os_ask_virtual_memory_block_with_address(void* target_address, size_t block_bytes);
void* os_ask_heap_memory(size_t size);
size_t os_get_page_size(void);
