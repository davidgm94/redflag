//
// Created by david on 10/12/20.
//

#include "error_message.h"
#include "slice.h"
#include "os.h"

enum ErrorType
{
    ERROR_TYPE_ERROR,
    ERROR_TYPE_NOTE,
};

static void print_error_message_type(ErrorMessage* error, ErrorType error_type)
{
    if (error->path)
    {
        const char* path = buf_ptr(error->path);
        Slice<const char> path_slice = {path, strlen(path)};
        static Buffer* cwd_buffer = nullptr;
        static Slice<const char> cwd;

        if (!cwd_buffer)
        {
            cwd_buffer = buf_alloc();
            Error error = os_get_cwd(cwd_buffer);
            if (error != ERROR_NONE)
            {
                RED_PANIC("Get CWD failed!");
            }
            buf_append_char(cwd_buffer, RED_OS_SEP_CHAR);
            cwd.ptr = buf_ptr(cwd_buffer);
            cwd.len = strlen(cwd.ptr);
        }

        const size_t line = error->line_start + 1;
        const size_t column = error->column_start + 1;

        if (slice_starts_with(path_slice, cwd))
        {
            fprintf(stderr, ".%c%s:%" RED_PRI_usize ":%" RED_PRI_usize ": ", RED_OS_SEP_CHAR, path + cwd.len, line, column);
        }
        else
        {
            fprintf(stderr, "%s:%" RED_PRI_usize ":%" RED_PRI_usize ": ", path, line, column);
        }
    }

    switch (error_type)
    {
        case ERROR_TYPE_ERROR:
            fprintf(stdout, "error: ");
            break;
        case ERROR_TYPE_NOTE:
            fprintf(stdout, "note: ");
            break;
        default:
            RED_UNREACHABLE;
    }

    fputs(buf_ptr(error->message), stdout);
    fputc('\n', stdout);

    if (buf_len(&error->line_buffer) != 0)
    {
        fprintf(stdout, "%s\n", buf_ptr(&error->line_buffer));
        for (size_t i = 0; i < error->column_start; i++)
        {
            fprintf(stdout, " ");
        }

        fprintf(stdout, "^");
        fprintf(stdout, "\n");
    }

    for (size_t i = 0; i < error->notes.length; i++)
    {
        ErrorMessage* note = error->notes.at(i);
        print_error_message_type(note, ERROR_TYPE_NOTE);
    }
}

ErrorMessage* ErrorMessage_create_with_line(Buffer* path, size_t line, size_t column, Buffer* source, List<size_t>* line_offsets, Buffer* message)
{
    ErrorMessage* error_message = new_elements(ErrorMessage, 1);
    error_message->path = path;
    error_message->line_start = line;
    error_message->column_start = column;
    error_message->message = message;

    size_t line_start_offset = line_offsets->at(line);
    size_t end_line = line + 1;
    size_t line_end_offset = (end_line >= line_offsets->length) ? buf_len(source) : line_offsets->at(line + 1);
    size_t len = (line_end_offset + 1 > line_start_offset) ? (line_end_offset - line_start_offset - 1) : 0;
    if (len == SIZE_MAX)
    {
        len = 0;
    }

    buf_init_from_mem(&error_message->line_buffer, buf_ptr(source) + line_start_offset, len);

    return error_message;
}

void print_error_message(ErrorMessage*error)
{
    print_error_message_type(error, ERROR_TYPE_ERROR);
}

