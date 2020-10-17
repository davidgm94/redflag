//
// Created by david on 7/10/20.
//
#pragma once

void red_panic(const char* file, int line, const char* function, const char* format, ...);
#define RED_NOT_IMPLEMENTED red_panic(__FILE__, __LINE__, __func__, "Not implemented")
#define RED_UNREACHABLE red_panic(__FILE__, __LINE__, __func__, "Unreachable")
#define RED_PANIC(...) red_panic(__FILE__, __LINE__, __func__, __VA_ARGS__)
