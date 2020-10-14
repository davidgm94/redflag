//
// Created by david on 10/12/20.
//

#pragma once

#include "types.h"
#include "list.h"
#include "buffer.h"

struct ErrorMessage
{
    size_t line_start;
    size_t column_start;
    Buffer* message;
    Buffer* path;
    Buffer line_buffer;

    List<ErrorMessage*> notes;
};