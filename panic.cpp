//
// Created by david on 7/10/20.
//

#include "types.h"
#include "panic.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void red_panic(const char* file, int line, const char* function, const char* format, ...)
{
    char buffer[10 * 1024];
    //memset(buffer, 0x00, sizeof(buffer));

    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);

    fprintf(stdout, "Panic at %s:%zu: %s -> %s\n", file, line, function, buffer);
    fprintf(stdout, "\n");
    fflush(stdout);
    abort();
}
