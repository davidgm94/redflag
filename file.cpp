//
// Created by david on 7/10/20.
//

#include "file.h"
#include "types.h"
#include <stdio.h>
#include <assert.h>

static char src_buffer[10000];
char *src_it = &src_buffer[0];


char *file_load(const char *name)
{
    FILE *file = fopen(name, "rb");
    assert(file);

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    printf("[FILE] File length: %ld.\n", length);
    assert(0 < length && (src_it + length) < src_it + array_length(src_buffer));
    fseek(file, 0, SEEK_SET);

    size_t rc = fread(src_it, 1, length, file);
    assert(rc == (size_t) length);
    fclose(file);

    return src_it;
}
