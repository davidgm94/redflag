//
// Created by david on 10/12/20.
//

#include "compiler_types.h"
#include "lexer.h"
#include "os.h"
#include "src_file.h"
#if RED_LLVM_BACKEND
#elif RED_C_BACKEND
#elif RED_SELF_BACKEND
#else
#error
#endif

void add_source_file(SB* src_buffer, const char* path)
{
    //print("Src file:\n\n***\n\n%s\n\n***\n\n", src_buffer);
    LexingResult lexing_result = lex(src_buffer);
    if (lexing_result.error.len)
    {
        os_abort();
    }

#if RED_LEXER_VERBOSE
    print_tokens(src_buffer, &lexing_result.tokens);
#endif

    //List<RedAST::FunctionDefinition*> fn_list = parse(src_buffer, &lexing_result.tokens);
//#if RED_PARSER_VERBOSE
//    parser_print_ast(&fn_list);
//#endif
#if RED_LLVM_BACKEND
    //auto obj = llvm_codegen(&fn_list);
    //link_object(obj.c_str());
#elif RED_C_BACKEND
    c_codegen(&fn_list);
#elif RED_SELF_BACKEND
#endif


}
