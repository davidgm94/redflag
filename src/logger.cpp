//
// Created by david on 10/6/20.
//

#include "logger.h"
#include "types.h"
#include <stdio.h>
#include <stdarg.h>


static const char *log_type_to_str(LogType log_type)
{
    switch (log_type)
    {
        CASE_TO_STR(LOG_TYPE_INFO);
        CASE_TO_STR(LOG_TYPE_WARN);
        CASE_TO_STR(LOG_TYPE_ERROR);
        default:
            RED_UNREACHABLE;
    }
    return NULL;
}

void print(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
}

void logger(LogType log_type, const char *format, ...)
{
    fprintf(stdout, "[%s] ", log_type_to_str(log_type));
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
}

