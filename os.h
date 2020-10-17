//
// Created by david on 10/17/20.
//

#pragma once
#include <inttypes.h>

#if __linux__
#define RED_OS_LINUX
#else
#error
#endif

#include "buffer.h"
#include "error.h"

#ifdef RED_OS_LINUX
#define RED_PRI_usize "zu"
#define RED_PRI_s64 PRId64
#define RED_PRI_u64 PRIu64
#define RED_PRI_llu "llu"
#define OS_SEP "/"
#define RED_OS_SEP_CHAR '/'
Error os_get_cwd(Buffer* out_cwd);
#endif
