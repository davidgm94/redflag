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

void add_source_file(SB* src_buffer, const char* filename)
{
#if RED_SRC_FILE_VERBOSE
    print("Src file:\n\n***\n\n%s\n\n***\n\n", sb_ptr(src_buffer));
#endif
    ExplicitTimer lexer_dt = os_timer_start("Lexer");
    LexingResult lexing_result = lex_file(src_buffer);
    f64 lexer_ms = os_timer_end(&lexer_dt);
    u64 byte_count = src_buffer->len;
    f64 byte_per_ms = byte_count / lexer_ms;
    f64 byte_per_s = byte_per_ms * 1000;
    print("Bytes: %llu\nTime in ms: %f\nLexer throughput: %Lf bytes/s\nLexer throughput: %Lf MB/s\n\n", byte_count, lexer_ms, byte_per_s, byte_per_s / 1000000);
    if (lexing_result.error.len)
    {
        os_exit(1);
    }

#if RED_LEXER_VERBOSE
    print_tokens(src_buffer, &lexing_result.tokens);
#endif

    ExplicitTimer parser_dt = os_timer_start("Parse");
    RedAST ast = parse_translation_unit(src_buffer, &lexing_result.tokens, filename);
    os_timer_end(&parser_dt);

    ExplicitTimer ir_dt = os_timer_start("IRGen");
    IRModule ir_tree = transform_ast_to_ir(&ast);
    os_timer_end(&ir_dt);

    llvm_gen_machine_code(&ir_tree);
}
