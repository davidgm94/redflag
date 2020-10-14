//
// Created by david on 10/6/20.
//

#pragma once
#include "types.h"

enum LogType
{
    LOG_TYPE_INFO,
    LOG_TYPE_WARN,
    LOG_TYPE_ERROR,
};

void logger(LogType log_type, const char *format, ...);
