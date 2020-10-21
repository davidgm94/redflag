//
// Created by david on 10/16/20.
//

#include "ast_render.h"
#include "compiler_types.h"
#include "red_parser.h"

struct ASTPrint
{
    s32 indent;
    FILE* file;
};

static const char *node_type_str(NodeType node_type)
{
    switch (node_type)
    {
        CASE_TO_STR(NODE_TYPE_FN_PROTO);
        CASE_TO_STR(NODE_TYPE_FN_DEF);
        CASE_TO_STR(NODE_TYPE_PARAM_DECL);
        CASE_TO_STR(NODE_TYPE_BLOCK);
        CASE_TO_STR(NODE_TYPE_GROUPED_EXPR);
        CASE_TO_STR(NODE_TYPE_RETURN_EXPR);
        CASE_TO_STR(NODE_TYPE_VARIABLE_DECLARATION);
        CASE_TO_STR(NODE_TYPE_BIN_OP_EXPR);
        CASE_TO_STR(NODE_TYPE_FLOAT_LITERAL);
        CASE_TO_STR(NODE_TYPE_INT_LITERAL);
        CASE_TO_STR(NODE_TYPE_STRING_LITERAL);
        CASE_TO_STR(NODE_TYPE_CHAR_LITERAL);
        CASE_TO_STR(NODE_TYPE_SYMBOL);
        CASE_TO_STR(NODE_TYPE_PREFIX_OP_EXPR);
        CASE_TO_STR(NODE_TYPE_POINTER_TYPE);
        CASE_TO_STR(NODE_TYPE_FN_CALL_EXPR);
        CASE_TO_STR(NODE_TYPE_ARRAY_ACCESS_EXPR);
        CASE_TO_STR(NODE_TYPE_FIELD_ACCESS_EXPR);
        CASE_TO_STR(NODE_TYPE_PTR_DEREF);
        CASE_TO_STR(NODE_TYPE_BOOL_LITERAL);
        CASE_TO_STR(NODE_TYPE_NULL_LITERAL);
        CASE_TO_STR(NODE_TYPE_UNDEFINED_LITERAL);
        CASE_TO_STR(NODE_TYPE_UNREACHABLE);
        CASE_TO_STR(NODE_TYPE_IF_BOOL_EXPR);
        CASE_TO_STR(NODE_TYPE_WHILE_EXPR);
        CASE_TO_STR(NODE_TYPE_FOR_EXPR);
        CASE_TO_STR(NODE_TYPE_SWITCH_EXPR);
        CASE_TO_STR(NODE_TYPE_BREAK);
        CASE_TO_STR(NODE_TYPE_CONTINUE);
        CASE_TO_STR(NODE_TYPE_CONTAINER_DECL);
        CASE_TO_STR(NODE_TYPE_STRUCT_FIELD);
        CASE_TO_STR(NODE_TYPE_ENUM_LITERAL);
        CASE_TO_STR(NODE_TYPE_TYPE);
    }

    return nullptr;
}

static void AST_print_visit(ASTNode** node_ptr, void* context)
{
    ASTNode* node = *node_ptr;
    ASTPrint* ap = (ASTPrint*)context;

    for (s32 i = 0; i < ap->indent; i++)
    {
        fprintf(ap->file, " ");
    }

    fprintf(ap->file, "%s\n", node_type_str(node->type));

    ASTPrint new_ap;
    new_ap.indent = ap->indent + 2;
    new_ap.file = ap->file;

    red_parser_visit_node_children(node, AST_print_visit, &new_ap);
}
void ast_print(FILE* file, ASTNode* node, s32 indent)
{
    ASTPrint ap;
    ap.indent = indent;
    ap.file = file;
    red_parser_visit_node_children(node, AST_print_visit, &ap);
}

void ast_render(FILE* file, ASTNode* node, s32 indent_size)
{

}
