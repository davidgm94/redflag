//
// Created by david on 7/10/20.
//

#include "error.h"
#include <assert.h>
#include <stdio.h>

void panic(const char *message)
{
    fprintf(stdout, "[PANIC] %s\n", message);
    assert(0);
}
