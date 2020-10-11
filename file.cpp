//
// Created by david on 7/10/20.
//

#include "file.h"
#include "types.h"
#include "buffer.h"
#include <stdio.h>
#include <assert.h>

Buffer file_load(const char *name)
{
    Buffer file_buffer = {0};
    FILE *file = fopen(name, "rb");
    assert(file);

    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    printf("[FILE] File length: %ld.\n", length);
    buf_resize(&file_buffer, length);
    fseek(file, 0, SEEK_SET);

    size_t rc = fread(file_buffer.items, 1, length, file);
    assert(rc == (size_t) length);
    fclose(file);

    return file_buffer;
}
