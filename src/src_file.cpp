//
// Created by david on 10/12/20.
//

#include "compiler_types.h"
#include "src_file.h"
#include "lexer.h"
#include "red_parser.h"
#include "ast_render.h"
#include "codegen.h"
#include "new_parser.h"
#include "llvm_codegen.h"

static RedType* get_root_container_type(const char* name, RootStruct* root_struct)
{
    RedType* entry = NEW<RedType>(1);
    entry->id = RED_TYPE_STRUCT;
    buf_init_from_str(&entry->name, name);
    entry->structure.root_struct = root_struct;

    return entry;
}


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

#if RED_NEW_PARSER // Parser and codegen
    List<RedAST::Function*> fn_list = new_parse(src_buffer, &lexing_result.tokens);
    llvm_codegen(&fn_list);
#else // Parser and codegen
    Buffer buffer = {};
    buf_init_from_str(&buffer, "main");
    RootStruct* root_struct = NEW<RootStruct>(1);
    root_struct->path = buf_alloc_fixed(strlen(path));
    buf_init_from_str(root_struct->path, path);
    root_struct->line_offsets = &lexing_result.line_offsets;
    root_struct->src_code = source_code;
    RedType* root_struct_entry = get_root_container_type("root", root_struct);
    ASTNode* root_node = red_parse(source_code, &lexing_result.tokens, root_struct_entry);
    redassert(root_node);
    redassert(root_node->type == NODE_TYPE_CONTAINER_DECL);
    ast_print(stdout, root_node, 0);

    // LLVM
    CodeGenConfig cg_cfg = {};
    cg_cfg.is_static = true;
    cg_cfg.release = false;
    cg_cfg.strip_debug_symbols = false;

    Buffer output_file = {};
    buf_init_from_str(&output_file, "test");
    red_codegen_and_link(root_node, &cg_cfg, source_code, &output_file);
#endif


}
