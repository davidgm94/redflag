//
// Created by david on 10/12/20.
//

#include "compiler.h"
#include "compiler_types.h"
#include "os.h"
#include "lexer.h"
#include "parser.h"
#include "ir.h"
#include "bytecode.h"
#include "llvm.h"

typedef struct CompilerWorkQueue CompilerWorkQueue;

typedef void CompilerTaskDescription(CompilerWorkQueue*queue, void*data);

typedef struct CompilerTask
{
    CompilerTaskDescription*task_fn;
    void*data;
} CompilerTask;
#define TASK_COUNT 256

typedef struct TaskQueue
{
    CompilerTask tasks[TASK_COUNT];
    OS_Semaphore sem;
    volatile u32 completion_goal;
    volatile u32 next_entry_to_read;
} TaskQueue, CompilerTaskQueue;

typedef struct IncludedFiles
{
    SBBuffer system_modules;
    SBBuffer user_modules;
} IncludedFiles;

static inline IncludedFiles collect_included_files(TokenBuffer*tb);
static inline ASTModuleBuffer load_lex_and_parse_included_modules(IncludedFiles* included_files);


void compile_program(SB* build_src_file_buffer)
{
#if RED_SRC_FILE_VERBOSE
    print("Src file:\n\n***\n\n%s\n\n***\n\n", sb_ptr(build_src_file_buffer));
#endif
    ExplicitTimer lexer_dt = os_timer_start("Lexer");
    SB module_sb = ZERO_INIT;
    sb_strcpy(&module_sb, "main");

    LexingResult lexing_result = lex_file(build_src_file_buffer);
    f64 lexer_ms = os_timer_end(&lexer_dt);
    u64 byte_count = build_src_file_buffer->len;
    f64 byte_per_ms = byte_count / lexer_ms;
    f64 byte_per_s = byte_per_ms * 1000;
    print("Bytes: %llu\nTime in ms: %f\nLexer throughput: %Lf bytes/s\nLexer throughput: %Lf MB/s\n\n", byte_count,
          lexer_ms, byte_per_s, byte_per_s / 1000000);
    if (lexing_result.error.len)
    {
        os_exit(1);
    }

#if RED_LEXER_VERBOSE
    print_tokens(build_src_file_buffer, &lexing_result.tokens);
#endif

    ExplicitTimer parser_dt = os_timer_start("Parse");
    IncludedFiles included_files = collect_included_files(&lexing_result.tokens);
    u32 system_module_count = included_files.system_modules.len;
    u32 user_module_count = included_files.user_modules.len;
    u32 total_module_count = system_module_count + user_module_count;
    if (total_module_count > 0)
    {
        load_lex_and_parse_included_modules(&included_files);
    }

    // Parse main module
    ASTModule ast = parse_module(&lexing_result.tokens, &module_sb);
    os_timer_end(&parser_dt);

    // TODO: commented for now to make parser changes
//    ExplicitTimer ir_dt = os_timer_start("IRGen");
//    IRModule ir_tree = transform_ast_to_ir(&ast);
//    os_timer_end(&ir_dt);

    // TODO: we are transitioning from a pseudo-IR into a bytecode
    //llvm_gen_machine_code(&ir_tree);
}

typedef struct ModuleThreadResult
{
    ASTModule module;
} ModuleThreadResult;


static inline IncludedFiles collect_included_files(TokenBuffer* tb)
{
    IncludedFiles files = ZERO_INIT;
    ParseContext main_pc = {.token_buffer = tb, .current_token = 0};
    // Import keyword is for system modules
    while (parse_file_load_or_import(&main_pc, &files.system_modules, "import"))
    { }
    // Load keyword is for user-level modules
    while (parse_file_load_or_import(&main_pc, &files.user_modules, "load"))
    { }
    return files;
}

static inline ASTModuleBuffer load_lex_and_parse_included_modules(IncludedFiles* included_files)
{
    ASTModuleBuffer module_buffer = ZERO_INIT;
    u32 system_module_count = included_files->system_modules.len;
    u32 user_module_count = included_files->user_modules.len;

    SB** system_module_it = included_files->system_modules.ptr;
    for (u32 i = 0; i < system_module_count; i++)
    {
        SB* module = system_module_it[i];
        sb_assert_not_empty(module);
        ASTModule ast_module = load_lex_and_parse_system_module(module);
        ast_append(&module_buffer, ast_module);
    }

    SB** user_module_it = included_files->user_modules.ptr;
    for (u32 i = 0; i < user_module_count; i++)
    {
        SB* module = user_module_it[i];
        ASTModule ast_module = load_lex_and_parse_user_module(module);
        ast_append(&module_buffer, ast_module);
    }

    return module_buffer;
}
