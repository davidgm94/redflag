//
// Created by david on 7/10/20.
//

#include "compiler_types.h"
#include "file.h"
#include <stdio.h>

// TODO: move to OS module

FileLoadResult file_load(const char *name, Buffer* file_buffer)
{
    s32 rc;
    FILE *file = fopen(name, "rb");
    if (!file)
        return FILE_LOAD_RESULT_ERROR;

    rc = fseek(file, 0, SEEK_END);
    if (rc) // not zero
    {
        return FILE_LOAD_RESULT_ERROR;
    }
    size_t length = ftell(file);
    memset(file_buffer, 0, sizeof(Buffer));
    buf_resize(file_buffer, length);
    if (!file_buffer->items)
        return FILE_LOAD_RESULT_ERROR;
    rc = fseek(file, 0, SEEK_SET);
    if (rc) // not zero
    {
        return FILE_LOAD_RESULT_ERROR;
    }

    rc = fread(file_buffer->items, 1, length, file);
    if (rc != (size_t) length)
    {
        return FILE_LOAD_RESULT_ERROR;
    }

    rc = fclose(file);
    if (rc) // not zero
    {
        return FILE_LOAD_RESULT_ERROR;
    }

    return FILE_LOAD_RESULT_SUCCESS;
}
