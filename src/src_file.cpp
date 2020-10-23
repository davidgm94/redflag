//
// Created by david on 10/12/20.
//

#include "compiler_types.h"
#include "src_file.h"
#include "lexer.h"
#include "red_parser.h"
#include "ast_render.h"
#include "config.h"

static RedType* get_root_container_type(const char* name, RootStruct* root_struct)
{
    RedType* entry = new_elements(RedType, 1);
    entry->id = RED_TYPE_STRUCT;
    buf_init_from_str(&entry->name, name);
    entry->data.structure.root_struct = root_struct;

    return entry;
}


void add_source_file(Buffer*source_code, const char* path)
{
    LexingResult lexing_result = lex(source_code);
    if (lexing_result.error)
    {
        RED_UNREACHABLE;
    }

#if LEXER_VERBOSE
    print_tokens(source_code, &lexing_result.tokens);
#endif

    Buffer buffer;
    buf_init_from_str(&buffer, "main");
    RootStruct* root_struct = new_elements(RootStruct, 1);
    root_struct->path = buf_alloc_fixed(strlen(path));
    buf_init_from_str(root_struct->path, path);
    root_struct->line_offsets = &lexing_result.line_offsets;
    root_struct->src_code = source_code;
    RedType* root_struct_entry = get_root_container_type("root", root_struct);
    ASTNode* root_node = red_parse(source_code, &lexing_result.tokens, root_struct_entry);
    assert(root_node);
    assert(root_node->type == NODE_TYPE_CONTAINER_DECL);
    ast_print(stdout, root_node, 0);
}
