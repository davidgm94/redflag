//
// Created by david on 7/10/20.
//
#pragma once
#include "buffer.h"

enum FileLoadResult
{
    FILE_LOAD_RESULT_ERROR,
    FILE_LOAD_RESULT_SUCCESS,
};

FileLoadResult file_load(const char *name, Buffer* file_buffer);
