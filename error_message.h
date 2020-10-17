//
// Created by david on 10/12/20.
//

#pragma once

#include "types.h"
#include "list.h"
#include "buffer.h"
#include "parser.h"

struct ErrorMessage
{
    size_t line_start;
    size_t column_start;
    Buffer* message;
    Buffer* path;
    Buffer line_buffer;

    List<ErrorMessage*> notes;
};

ErrorMessage* ErrorMessage_create_with_line(Buffer* path, size_t line, size_t column, Buffer* source, List<size_t>* line_offsets, Buffer* message);

void print_error_message(ErrorMessage* error, ErrorColor error_color);