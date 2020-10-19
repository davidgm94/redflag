//
// Created by david on 10/17/20.
//

#include "os.h"
#include "buffer.h"

#ifdef RED_OS_LINUX
#define RED_OS_POSIX
#include <unistd.h>
#include <linux/limits.h>
#elif defined RED_OS_WINDOWS
#include <Windows.h>
#endif

#include <errno.h>


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
    buf_init_from_str(out_cwd, buffer);

    return ERROR_NONE;
#endif
}
