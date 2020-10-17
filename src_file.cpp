//
// Created by david on 10/12/20.
//

#include "buffer.h"
#include "src_file.h"
#include "lexer.h"
#include "parser.h"

void
add_source_file(CodeGen*code_gen, RedPackage*red_package, Buffer*resolved_path, Buffer*source_code, SourceType src_type)
{
    LexingResult lexing_result = lex(source_code);
    if (lexing_result.error)
    {
        RED_UNREACHABLE;
    }

    print_tokens(source_code, &lexing_result.tokens);

    ASTNode* root_node = parse(source_code, &lexing_result.tokens, NULL, ERROR_COLOR_ON);
    assert(root_node);
    assert(root_node->type == NODE_TYPE_CONTAINER_DECL);
    ast_print(stdout, root_node, 0);
}
