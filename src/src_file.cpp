//
// Created by david on 10/12/20.
//

#include "compiler_types.h"
#include "src_file.h"
#include "lexer.h"
#include "parser.h"
#include "llvm_codegen.h"

void add_source_file(Buffer* src_buffer, const char* path)
{
    LexingResult lexing_result = lex(src_buffer);
    if (lexing_result.error)
    {
        os_abort();
    }

#if RED_LEXER_VERBOSE
    print_tokens(source_code, &lexing_result.tokens);
#endif

    List<RedAST::Function*> fn_list = parse(src_buffer, &lexing_result.tokens);
#if RED_PARSER_VERBOSE
    parser_print_ast(&fn_list);
#endif
    llvm_codegen(&fn_list);


}
