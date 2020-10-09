//
// Created by david on 7/10/20.
//

#include <errno.h>
#include <assert.h>
#include <string.h>
#include "error.h"
#include "bigint.h"
#include "bigfloat.h"

/* TODO: do right */
f128 parse_f128(char *begin, char **p_string)
{
    NOT_IMPLEMENTED;
    return float128_t();
}

/* TODO: Simplify */
Error BigFloat_init_buffer(BigFloat *dst, const u8 *buffer, size_t buffer_length)
{
    char* str_begin = (char*)buffer;
    char* str_end;

    errno = 0;
    dst->value = parse_f128(str_begin, &str_end);
    if (errno)
    {
        return ERROR_OVERFLOW;
    }

    assert(str_end <= ((char*)buffer) + buffer_length);
    return ERROR_NONE;
}

void f32_to_f128M(f32 value, f128 *ptr)
{
    NOT_IMPLEMENTED;
}

void BigFloat_init_32(BigFloat* dst, f32 x)
{
    f32 f32_value;
    memcpy(&f32_value, &x, sizeof(f32));
    f32_to_f128M(f32_value, &dst->value);
}
