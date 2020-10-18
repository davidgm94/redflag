//
// Created by david on 10/18/20.
//

#include "red_parser.h"
#include "compiler_types.h"
#include "lexer.h"
#include "buffer.h"
#include "error_message.h"
#include <stdlib.h>
#include <stdarg.h>

struct ParseContext
{
    Buffer* buffer;
    List<Token>* tokens;
    size_t current_token;
    RedType* owner;
};

struct PointerPayload
{
    Token* asterisk;
    Token* payload;
};

struct PointerIndexPayload
{
    Token* asterisk;
    Token* payload;
    Token* index;
};


void red_parser_print(ASTNode* node, s32 indent)
{

}

void red_parser_visit_node_children(ASTNode* node, void (*visit)(ASTNode**, void*), void* context)
{

}

namespace AST
{
    static void error(ParseContext* pc, Token* token, const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        Buffer* message = buf_vprintf(format, args);
        va_end(args);

        ErrorMessage* error = ErrorMessage_create_with_line(pc->owner->data.structure.root_struct->path, token->start_line, token->start_column, pc->owner->data.structure.root_struct->src_code, pc->owner->data.structure.root_struct->line_offsets, message);
        error->line_start = token->start_line;
        error->column_start = token->start_column;

        print_error_message(error);
        exit(-1);
    }
    static inline void invalid_token_error(ParseContext* pc, Token* token)
    {
        error(pc, token, "invalid token: '%s'", token_name(token->id));
    }

    static inline ASTNode* create_node_no_line_info(ParseContext* pc, NodeType type)
    {
        ASTNode* node = new_elements(ASTNode, 1);
        node->type = type;
        node->owner = pc->owner;
        return node;
    }

    static ASTNode* create_node(ParseContext* pc, NodeType type, Token* first_token)
    {
        assert(first_token);
        ASTNode* node = create_node_no_line_info(pc, type);
        node->line = first_token->start_line;
        node->column = first_token->start_column;
        return node;
    }

    static ASTNode* create_node_copy_line_info(ParseContext* pc, NodeType type, ASTNode* from)
    {
        assert(from);
        ASTNode* node = create_node_no_line_info(pc, type);
        node->line = from->line;
        node->column = from->column;
        return node;
    }

    static inline Token* get_token_i(ParseContext* pc, size_t i)
    {
        return &pc->tokens->at(pc->current_token + i);
    }

    static inline Token* get_token(ParseContext* pc)
    {
        return get_token_i(pc, 0);
    }

    static inline Token* consume_token(ParseContext* pc)
    {
        Token* eaten = get_token(pc);
        pc->current_token += 1;
        return eaten;
    }

    static inline Token* consume_token_if(ParseContext* pc, TokenID id)
    {
        Token* eaten = get_token(pc);
        if (eaten->id == id)
        {
            return consume_token(pc);
        }
        return nullptr;
    }

    static inline Token* expect_token(ParseContext* pc, TokenID id)
    {
        Token* token = get_token(pc);
        if (token->id != id)
        {
            error(pc, token, "Expected token '%s', found '%s'", token_name(id), token_name(token->id));
        }

        return token;
    }

    static inline void put_back_token(ParseContext* pc)
    {
        pc->current_token -= 1;
    }

    static inline Buffer* token_buffer(Token* token)
    {
        if (token == nullptr)
        {
            return nullptr;
        }
        assert(token->id == TOKEN_ID_STRING_LIT || token->id == TOKEN_ID_MULTILINE_STRING_LIT || token->id == TOKEN_ID_SYMBOL);
        return &token->data.str_lit.str;
    }

    static inline BigInt* token_bigint(Token* token)
    {
        if (token == nullptr)
        {
            return nullptr;
        }
        assert(token->id == TOKEN_ID_INT_LIT);
        return &token->data.int_lit.big_int;
    }

    static inline ASTNode* token_symbol(ParseContext* pc, Token* token)
    {
        assert(token->id == TOKEN_ID_SYMBOL);
        ASTNode* node = create_node(pc, NODE_TYPE_SYMBOL, token);
        node->data.symbol_expr.symbol = token_buffer(token);
        return node;
    }

    static ASTNodeContainerDeclaration parse_container_members(ParseContext* pc);

    static inline ASTNode* parse_container_declaration(ParseContext* pc)
    {
        ContainerType type;
        Token* container_type_token = consume_token_if(pc, TOKEN_ID_KEYWORD_STRUCT);
        if (container_type_token)
        {
            type = CONTAINER_TYPE_STRUCT;
            goto start;
        }

        container_type_token = consume_token_if(pc, TOKEN_ID_KEYWORD_ENUM);
        if (container_type_token)
        {
            type = CONTAINER_TYPE_ENUM;
            goto start;
        }

        container_type_token = consume_token_if(pc, TOKEN_ID_KEYWORD_UNION);
        if (container_type_token)
        {
            type = CONTAINER_TYPE_UNION;
            goto start;
        }

        return nullptr;
    start:
        ASTNode* container_node = create_node(pc, NODE_TYPE_CONTAINER_DECL, container_type_token);
        container_node->data.container_decl.type = type;

        Token* token = expect_token(pc, TOKEN_ID_SYMBOL);
        container_node->data.container_decl.name = token_buffer(token);

        expect_token(pc, TOKEN_ID_LEFT_BRACE);
        ASTNodeContainerDeclaration members = parse_container_members(pc);
//        if (!members)
//        {
//            return nullptr;
//        }
        expect_token(pc, TOKEN_ID_RIGHT_BRACE);

        container_node->data.container_decl.fields = members.fields;
        container_node->data.container_decl.declarations = members.declarations;

        return container_node;
    }

    static inline ASTNode* parse_variable_declaration(ParseContext* pc)
    {
        RED_NOT_IMPLEMENTED;
    }
    static inline ASTNode* parse_symbol_assignment(ParseContext* pc)
    {
        RED_NOT_IMPLEMENTED;
    }
    static inline ASTNode* parse_function_declaration(ParseContext* pc)
    {
        RED_NOT_IMPLEMENTED;
    }

    static inline ASTNode* parse_top_level_declaration(ParseContext* pc)
    {
        Token* token = get_token(pc);
        if (token->id == TOKEN_ID_SYMBOL)
        {
            ASTNode* node = parse_container_declaration(pc);
            if (node)
            {
                return node;
            }

            node = parse_variable_declaration(pc);
            if (node)
            {
                return node;
            }

            node = parse_symbol_assignment(pc);
            if (node)
            {
                return node;
            }

            node = parse_function_declaration(pc);
            if (node)
            {
                return node;
            }
        }

        return nullptr;
    }

    static ASTNodeContainerDeclaration parse_container_members(ParseContext* pc)
    {
        ASTNodeContainerDeclaration result = {};

        while (true)
        {
            ASTNode* top_level_declaration = parse_top_level_declaration(pc);
            if (top_level_declaration)
            {

            }
            else
            {
                RED_NOT_IMPLEMENTED;
            }
        }
    }
    static inline ASTNode* parse_root(ParseContext* pc)
    {
        ASTNodeContainerDeclaration root_members = parse_container_members(pc);
        if (pc->current_token != pc->tokens->length - 1)
        {
            invalid_token_error(pc, get_token(pc));
        }
    }
}

ASTNode* red_parse(Buffer*buffer, List<Token>*tokens, RedType*owner)
{
    ParseContext pc = {};
    pc.tokens = tokens;
    pc.buffer = buffer;
    pc.owner = owner;
    return AST::parse_root(&pc);
}

