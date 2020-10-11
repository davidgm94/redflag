//
// Created by David on 04/10/2020.
//

#include "types.h"
#include "lexer.h"
#include "file.h"

s32 main(s32 argc, char* argv[])
{
    size_t length;
    Buffer src_buffer = file_load("test.red");
    LexingResult lexer = lex(&src_buffer);
    print_tokens(&src_buffer, &lexer.tokens);
    return 0;
}