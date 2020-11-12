//
// Created by david on 10/12/20.
//

#include "compiler_types.h"
#include "src_file.h"
#include "os.h"
#include "lexer.h"
#include "parser.h"
#include "ir.h"
#include "llvm.h"

void add_source_file(SB* src_buffer, const char* path)
{
    print("Src file:\n\n***\n\n%s\n\n***\n\n", sb_ptr(src_buffer));
    ExplicitTimer lexer_dt = os_timer_start("Lexer");
    LexingResult lexing_result = lex_file(src_buffer);
    os_timer_end(&lexer_dt);
    if (lexing_result.error.len)
    {
        os_exit(1);
    }

#if RED_LEXER_VERBOSE
    print_tokens(src_buffer, &lexing_result.tokens);
#endif

    ExplicitTimer parser_dt = os_timer_start("Parse");
    RedModuleTree ast = parse_translation_unit(src_buffer, &lexing_result.tokens);
    os_timer_end(&parser_dt);

    ExplicitTimer ir_dt = os_timer_start("IRGen");
    RedModuleIR ir_tree = transform_ast_to_ir(&ast);
    os_timer_end(&ir_dt);

    ExplicitTimer llvm_dt = os_timer_start("LLVM");
    llvm_gen_machine_code(&ir_tree);
    os_timer_end(&llvm_dt);

//#if RED_LLVM_BACKEND
//    llvm_
//#elif RED_C_BACKEND
//    c_codegen(&fn_list);
//#elif RED_SELF_BACKEND
//#endif


}
