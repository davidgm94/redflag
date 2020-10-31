//
// Created by david on 7/10/20.
//

#include "types.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <Windows.h>

// TODO: move to OS
void red_panic(const char* file, size_t line, const char* function, const char* format, ...)
{
    char buffer[10 * 1024];
    char buffer2[10 * 1024];

    //memset(buffer, 0x00, sizeof(buffer));

    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);

    sprintf(buffer2, "Panic at %s:%zu: %s -> %s\n", file, line, function, buffer);
    MessageBoxA(GetActiveWindow(), buffer2, "PANIC", MB_ABORTRETRYIGNORE);
}