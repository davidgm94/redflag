//
// Created by david on 10/18/20.
//

#if 0

#include "red_parser.h"
#include "compiler_types.h"
#include "lexer.h"
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

namespace RedAST
{
    static inline Buffer* token_void_buffer()
    {
        static Buffer* void_buffer;
        if (!void_buffer)
        {
            void_buffer = buf_alloc();
            buf_init_from_str(void_buffer, "void");
        }
        return void_buffer;
    }

    static inline Buffer* token_buffer(Token* token)
    {
        if (token == nullptr)
        {
            return nullptr;
        }
        redassert(token->id == TOKEN_ID_STRING_LIT || token->id == TOKEN_ID_MULTILINE_STRING_LIT || token->id == TOKEN_ID_SYMBOL);
        return &token->str_lit.str;
    }

    static void error(ParseContext* pc, Token* token, const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        Buffer* message = buf_vprintf(format, args);
        va_end(args);

        ErrorMessage* error = ErrorMessage_create_with_line(pc->owner->structure.root_struct->path, token->start_line, token->start_column, pc->owner->structure.root_struct->src_code, pc->owner->structure.root_struct->line_offsets, message);
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
        ASTNode* node = NEW<ASTNode>(1);
        node->type = type;
        node->owner = pc->owner;
        return node;
    }

    static ASTNode* create_node(ParseContext* pc, NodeType type, Token* first_token)
    {
        redassert(first_token);
        ASTNode* node = create_node_no_line_info(pc, type);
        node->line = first_token->start_line;
        node->column = first_token->start_column;
        return node;
    }

    static ASTNode* create_node_copy_line_info(ParseContext* pc, NodeType type, ASTNode* from)
    {
        redassert(from);
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
        Token* token = get_token(pc);

#if PARSER_VERBOSE
        PRINT_TOKEN_WITH_PREFIX("Consuming", token, symbol_name);
#endif
        pc->current_token += 1;
        return token;
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

    static inline void expect_and_consume_token(ParseContext* pc, TokenID id)
    {
        Token* token = get_token(pc);
        if (!token)
        {
            error(pc, token, "Expected token '%s', found '%s'", token_name(id), token_name(token->id));
        }
        consume_token_if(pc, id);
    }

    static inline void put_back_token(ParseContext* pc)
    {
        Token* wrong_token = get_token(pc);
        pc->current_token -= 1;
        Token* good_token = get_token(pc);
#if PARSER_VERBOSE
        Buffer* wrong_symbol = wrong_token->id == TOKEN_ID_SYMBOL ? token_buffer(wrong_token) : nullptr;
        Buffer* good_symbol = good_token->id == TOKEN_ID_SYMBOL ? token_buffer(good_token) : nullptr;
        print("Current token #%zu: %s name: %s ******** Putting back token #%zu: %s name: %s\n", pc->current_token + 1, token_name(wrong_token->id), wrong_symbol ? wrong_symbol->items : "not a symbol", pc->current_token, token_name(good_token->id), good_symbol ? good_symbol->items : "not a symbol");
#endif
    }

    static inline BigInt* token_bigint(Token* token)
    {
        if (token == nullptr)
        {
            return nullptr;
        }
        redassert(token->id == TOKEN_ID_INT_LIT);
        return &token->int_lit.big_int;
    }

    static inline BigFloat* token_bigfloat(Token* token)
    {
        if (token == nullptr)
        {
            return nullptr;
        }
        redassert(token->id == TOKEN_ID_FLOAT_LIT);
        return &token->float_lit.big_float;
    }
    
    static inline ASTNode* token_type_symbol(ParseContext* pc, Token* token)
    {
        redassert(token->id == TOKEN_ID_SYMBOL);
        ASTNode* node = create_node(pc, NODE_TYPE_TYPE, token);
        node->type_expr.type = token_buffer(token);
        return node;
    }

    static inline ASTNode* token_symbol(ParseContext* pc, Token* token)
    {
        redassert(token->id == TOKEN_ID_SYMBOL);
        ASTNode* node = create_node(pc, NODE_TYPE_SYMBOL, token);
        node->symbol_expr.symbol = token_buffer(token);
        return node;
    }

    static inline ASTNode* expect(ParseContext* pc, ASTNode* (*parser)(ParseContext*))
    {
        ASTNode* result = parser(pc);
        if (!result)
        {
            invalid_token_error(pc, get_token(pc));
        }

        return result;
    }

    static inline bool is_container_keyword(TokenID id)
    {
        return id == TOKEN_ID_KEYWORD_STRUCT || id == TOKEN_ID_KEYWORD_ENUM || id == TOKEN_ID_KEYWORD_UNION;
    }

    static inline bool is_variable_keyword(TokenID id)
    {
        return id == TOKEN_ID_KEYWORD_VAR || id == TOKEN_ID_KEYWORD_CONST;
    }

    static ASTNodeContainerDeclaration parse_container_members(ParseContext* pc);


    //static inline ASTNode* parse_assign_op(ParseContext* pc)
    //{
    //    BinaryOpType table[256] = {};
    //    table[TOKEN_ID_BIT_AND_EQ] = BIN_OP_TYPE_ASSIGN_BIT_AND;
    //    table[TOKEN_ID_BIT_OR_EQ] = BIN_OP_TYPE_ASSIGN_BIT_OR;
    //    table[TOKEN_ID_BIT_SHL_EQ] = BIN_OP_TYPE_ASSIGN_BIT_SHIFT_LEFT;
    //    table[TOKEN_ID_BIT_SHR_EQ] = BIN_OP_TYPE_ASSIGN_BIT_SHIFT_RIGHT;
    //    table[TOKEN_ID_BIT_XOR_EQ] = BIN_OP_TYPE_ASSIGN_BIT_XOR;
    //    table[TOKEN_ID_DIV_EQ] = BIN_OP_TYPE_ASSIGN_DIV;
    //    table[TOKEN_ID_EQ] = BIN_OP_TYPE_ASSIGN;
    //    table[TOKEN_ID_MINUS_EQ] = BIN_OP_TYPE_ASSIGN_MINUS;
    //    table[TOKEN_ID_MOD_EQ] = BIN_OP_TYPE_ASSIGN_MOD;
    //    table[TOKEN_ID_PLUS_EQ] = BIN_OP_TYPE_ASSIGN_PLUS;
    //    table[TOKEN_ID_TIMES_EQ] = BIN_OP_TYPE_ASSIGN_TIMES;

    //    BinaryOpType op = table[get_token(pc)->id];
    //    if (op != BIN_OP_TYPE_INVALID)
    //    {
    //        Token* op_token = consume_token(pc);
    //        ASTNode* result = create_node(pc, NODE_TYPE_BIN_OP_EXPR, op_token);
    //        result->bin_op_expr.op = op;
    //        
    //        return result;
    //    }

    //    return nullptr;
    //}

    //static inline ASTNode* parse_bin_op_expression(ParseContext* pc, BinaryOpChain chain, ASTNode* (*op_parse)(ParseContext*), ASTNode* (*child_parse)(ParseContext*))
    //{
    //    ASTNode* result = child_parse(pc);

    //    if (!result)
    //    {
    //        return nullptr;
    //    }

    //    do
    //    {
    //        ASTNode* op = op_parse(pc);
    //        if (!op)
    //        {
    //            return nullptr;
    //        }

    //        ASTNode* left = result;
    //        ASTNode* right = expect(pc, child_parse);
    //        result = op;

    //        switch (op->type)
    //        {
    //            case NODE_TYPE_BIN_OP_EXPR:
    //                op->bin_op_expr.op1 = left;
    //                op->bin_op_expr.op2 = right;
    //                break;
    //            default:
    //                RED_UNREACHABLE;
    //        }
    //    } while (chain == BINARY_OP_CHAIN_INFINITE);

    //    return result;
    //}

    //static inline ASTNode* parse_compare_op(ParseContext* pc)
    //{
    //    BinaryOpType table[256];
    //    table[TOKEN_ID_CMP_EQ] = BIN_OP_TYPE_CMP_EQ;
    //    table[TOKEN_ID_CMP_NOT_EQ] = BIN_OP_TYPE_CMP_NOT_EQ;
    //    table[TOKEN_ID_CMP_LESS] = BIN_OP_TYPE_CMP_LESS;
    //    table[TOKEN_ID_CMP_GREATER] = BIN_OP_TYPE_CMP_GREATER;
    //    table[TOKEN_ID_CMP_LESS_OR_EQ] = BIN_OP_TYPE_CMP_LESS_OR_EQ;
    //    table[TOKEN_ID_CMP_GREATER_OR_EQ] = BIN_OP_TYPE_CMP_GREATER_OR_EQ;

    //    BinaryOpType op = table[get_token(pc)->id];
    //    if (op != BIN_OP_TYPE_INVALID)
    //    {
    //        Token* op_token = consume_token(pc);
    //        ASTNode* result = create_node(pc, NODE_TYPE_BIN_OP_EXPR, op_token);
    //        result->bin_op_expr.op = op;

    //        return result;
    //    }

    //    return nullptr;
    //}

    //static inline ASTNode* parse_bitwise_expression(ParseContext* pc)
    //{
    //    BinaryOpType table[256];
    //    table[TOKEN_ID_AMPERSAND] = BIN_OP_TYPE_BIN_AND;
    //    table[TOKEN_ID_BIT_XOR] = BIN_OP_TYPE_BIN_XOR;
    //    table[TOKEN_ID_BIT_OR] = BIN_OP_TYPE_BIN_OR;

    //    BinaryOpType op = table[get_token(pc)->id];

    //    if (op != BIN_OP_TYPE_INVALID)
    //    {
    //        Token* op_token = consume_token(pc);
    //        ASTNode* result = create_node(pc, NODE_TYPE_BIN_OP_EXPR, op_token);
    //        result->bin_op_expr.op = op;

    //        return result;
    //    }

    //    return nullptr;
    //}

    //static inline ASTNode* parse_compare_expression(ParseContext* pc)
    //{
    //    return parse_bin_op_expression(pc, BINARY_OP_CHAIN_INFINITE, parse_compare_op, parse_bitwise_expression);
    //}

    template <TokenID id, BinaryOpType op>
    static inline ASTNode* parse_bin_op_simple(ParseContext* pc)
    {
        Token* token = consume_token_if(pc, id);
        if (!token)
        {
            return nullptr;
        }

        ASTNode* result = create_node(pc, NODE_TYPE_BIN_OP_EXPR, token);
        result->bin_op_expr.op = op;

        return result;
    }

    //static inline ASTNode* parse_bool_and_expression(ParseContext* pc)
    //{
    //    return parse_bin_op_expression(pc, BINARY_OP_CHAIN_INFINITE,
    //        parse_bin_op_simple<TOKEN_ID_KEYWORD_AND, BIN_OP_TYPE_BOOL_AND>,
    //        parse_compare_expression);
    //}

    //static inline ASTNode* parse_primary_type_expression(ParseContext* pc)
    //{
    //    Token*
    //}

    //static inline ASTNode* parse_type_expression(ParseContext* pc)
    //{
    //    return expect(pc, parse_primary_type_expression);
    //}

    //static inline ASTNode* parse_expression(ParseContext* pc)
    //{
    //    return parse_bin_op_expression(pc, BINARY_OP_CHAIN_INFINITE,
    //        parse_bin_op_simple<TOKEN_ID_KEYWORD_OR, BIN_OP_TYPE_BOOL_OR>,
    //        parse_bool_and_expression);
    //}

    static inline ASTNode* parse_container_declaration(ParseContext* pc)
    {
        ContainerType type_kind;
        Token* container_type_token = consume_token_if(pc, TOKEN_ID_KEYWORD_STRUCT);
        if (container_type_token)
        {
            type_kind = CONTAINER_TYPE_STRUCT;
            goto start;
        }

        container_type_token = consume_token_if(pc, TOKEN_ID_KEYWORD_ENUM);
        if (container_type_token)
        {
            type_kind = CONTAINER_TYPE_ENUM;
            goto start;
        }

        container_type_token = consume_token_if(pc, TOKEN_ID_KEYWORD_UNION);
        if (container_type_token)
        {
            type_kind = CONTAINER_TYPE_UNION;
            goto start;
        }

        return nullptr;
    start:
        ASTNode* container_node = create_node(pc, NODE_TYPE_CONTAINER_DECL, container_type_token);
        container_node->container_decl.type_kind = type_kind;

        Token* token = expect_token(pc, TOKEN_ID_SYMBOL);
        consume_token(pc);
        ASTNode* type_node = token_type_symbol(pc, token);
        container_node->container_decl.type_node = type_node;

        expect_and_consume_token(pc, TOKEN_ID_LEFT_BRACE);
        ASTNodeContainerDeclaration members = parse_container_members(pc);
        //if (!members)
        //{
        //    RED_NOT_IMPLEMENTED;
        //}
        expect_and_consume_token(pc, TOKEN_ID_RIGHT_BRACE);

        container_node->container_decl.fields = members.fields;
        container_node->container_decl.declarations = members.declarations;

        return container_node;
    }

    static inline ASTNode* parse_variable_declaration(ParseContext* pc)
    {
        Token* mutable_token = consume_token_if(pc, TOKEN_ID_KEYWORD_VAR);
        if (!mutable_token)
        {
            mutable_token = consume_token_if(pc, TOKEN_ID_KEYWORD_CONST);
        }
        if (mutable_token)
        {
            Token* name = expect_token(pc, TOKEN_ID_SYMBOL);
            consume_token(pc);
            Token* type = expect_token(pc, TOKEN_ID_SYMBOL);
            consume_token(pc);
#if PARSER_VERBOSE
            print("Token name: %s\n", token_buffer(name)->items);
            print("Token type: %s\n", token_buffer(type)->items);
#endif
            expect_and_consume_token(pc, TOKEN_ID_SEMICOLON);

            ASTNode* type_node = token_type_symbol(pc, type);

            ASTNode* variable_node = create_node(pc, NODE_TYPE_VARIABLE_DECLARATION, name);
            variable_node->variable_declaration.type = type_node;
            variable_node->variable_declaration.expression = nullptr;
            variable_node->variable_declaration.symbol = token_buffer(name);
            variable_node->variable_declaration.is_mutable = mutable_token->id == TOKEN_ID_KEYWORD_VAR;

            return variable_node;
        }
        return nullptr;
    }

    static inline ASTNode* parse_struct_union_member(ParseContext* pc)
    {
        Token* member_name = consume_token_if(pc, TOKEN_ID_SYMBOL);
        if (!member_name)
        {
            return nullptr;
        }
        Token* type_token = consume_token_if(pc, TOKEN_ID_SYMBOL);
        if (!type_token)
        {
            put_back_token(pc);
            return nullptr;
        }

        expect_and_consume_token(pc, TOKEN_ID_SEMICOLON);

        ASTNode* result = create_node(pc, NODE_TYPE_STRUCT_FIELD, member_name);
        result->struct_field.name = token_buffer(member_name);
        result->struct_field.type = token_type_symbol(pc, type_token);
        result->struct_field.value = nullptr;

        return result;
    }

    static inline ASTNode* parse_right_value(ParseContext* pc)
    {
        Token* value = consume_token_if(pc, TOKEN_ID_SYMBOL);
        if (value)
        {
            return token_symbol(pc, value);
        }

        value = consume_token_if(pc, TOKEN_ID_CHAR_LIT);
        if (value)
        {
            ASTNode* result = create_node(pc, NODE_TYPE_CHAR_LITERAL, value);
            result->char_literal.value = value->char_lit.value;

            return result;
        }

        value = consume_token_if(pc, TOKEN_ID_FLOAT_LIT);
        if (value)
        {
            ASTNode* result = create_node(pc, NODE_TYPE_FLOAT_LITERAL, value);
            result->float_literal.big_float = &value->float_lit.big_float;
            result->float_literal.overflow = value->float_lit.overflow;

            return result;
        }

        value = consume_token_if(pc, TOKEN_ID_INT_LIT);
        if (value)
        {
            ASTNode* result = create_node(pc, NODE_TYPE_INT_LITERAL, value);
            result->int_literal.big_int = &value->int_lit.big_int;

            return result;
        }

        value = consume_token_if(pc, TOKEN_ID_KEYWORD_TRUE);
        if (value)
        {
            ASTNode* result = create_node(pc, NODE_TYPE_BOOL_LITERAL, value);
            result->bool_literal.value = true;
        }

        value = consume_token_if(pc, TOKEN_ID_KEYWORD_FALSE);
        if (value)
        {
            ASTNode* result = create_node(pc, NODE_TYPE_BOOL_LITERAL, value);
            result->bool_literal.value = false;
        }

        value = consume_token_if(pc, TOKEN_ID_KEYWORD_NULL);
        if (value)
        {
            ASTNode* result = create_node(pc, NODE_TYPE_NULL_LITERAL, value);
            return result;
        }

        value = consume_token_if(pc, TOKEN_ID_KEYWORD_UNDEFINED);
        if (value)
        {
            ASTNode* result = create_node(pc, NODE_TYPE_UNDEFINED_LITERAL, value);
            return result;
        }

        value = consume_token_if(pc, TOKEN_ID_STRING_LIT);
        if (value)
        {
            ASTNode* result = create_node(pc, NODE_TYPE_STRING_LITERAL, value);
            result->string_literal.buffer = token_buffer(value);

            return result;
        }

        return nullptr;
    }

    static inline ASTNode* parse_symbol_assignment(ParseContext* pc)
    {
#if 1
        Token* symbol_name = consume_token_if(pc, TOKEN_ID_SYMBOL);
        if (!symbol_name)
        {
            return nullptr;
        }
        expect_and_consume_token(pc, TOKEN_ID_EQ);

        ASTNode* right_value = parse_right_value(pc);

        expect_and_consume_token(pc, TOKEN_ID_SEMICOLON);

        ASTNode* result = create_node(pc, NODE_TYPE_VARIABLE_DECLARATION, symbol_name);
        result->bin_op_expr.op = BIN_OP_TYPE_ASSIGN;
        result->bin_op_expr.op1 = token_symbol(pc, symbol_name);
        result->bin_op_expr.op2 = right_value;
        
        return result;
#else
        return parse_bin_op_expression(pc, BINARY_OP_CHAIN_ONCE, parse_assign_op, parse_expression);
#endif
    }

    static inline List<ASTNode*> parse_parameter_declaration_list(ParseContext* pc, TokenID separation_token, TokenID end_token)
    {
        List<ASTNode*> result = {};
        Token* token = nullptr;
        // TODO: messy
        while (((token = get_token(pc))->id) != end_token)
        {
            Token* name = expect_token(pc, TOKEN_ID_SYMBOL);
            consume_token(pc);
            Token* type_token = expect_token(pc, TOKEN_ID_SYMBOL);
            consume_token(pc);

            ASTNode* param = create_node(pc, NODE_TYPE_PARAM_DECL, name);
            param->param_decl.name = token_buffer(name);
            param->param_decl.type = token_type_symbol(pc, type_token);
            result.append(param);

            token = get_token(pc);
            if (token->id != end_token)
            {
                expect_token(pc, separation_token);
                consume_token(pc);
                expect_token(pc, TOKEN_ID_SYMBOL);
            }
        }
        redassert(token->id == end_token);
        consume_token(pc);

        return result;
    }
    static inline ASTNode* parse_parameter(ParseContext* pc)
    {
        /*
        Value possibilities:
        - float literal
        - int literal
        - string literal
        - char literal
        - symbol
        */
        Token* token = consume_token_if(pc, TOKEN_ID_INT_LIT);
        if (token)
        {
            ASTNode* node = create_node(pc, NODE_TYPE_INT_LITERAL, token);
            node->int_literal.big_int = token_bigint(token);
            return node;
        }

        token = consume_token_if(pc, TOKEN_ID_FLOAT_LIT);
        if (token)
        {
            ASTNode* node = create_node(pc, NODE_TYPE_INT_LITERAL, token);
            node->float_literal.big_float = token_bigfloat(token);
            node->float_literal.overflow = token->float_lit.overflow;
            return node;
        }

        token = consume_token_if(pc, TOKEN_ID_STRING_LIT);
        if (token)
        {
            ASTNode* node = create_node(pc, NODE_TYPE_STRING_LITERAL, token);
            node->string_literal.buffer = &token->str_lit.str;
            return node;
        }

        token = consume_token_if(pc, TOKEN_ID_CHAR_LIT);
        if (token)
        {
            ASTNode* node = create_node(pc, NODE_TYPE_CHAR_LITERAL, token);
            node->char_literal.value = token->char_lit.value;
            return node;
        }

        token = consume_token_if(pc, TOKEN_ID_SYMBOL);
        if (token)
        {
            return token_symbol(pc, token);
        }

        return nullptr;
    }
    static inline List<ASTNode*> parse_parameter_list(ParseContext* pc, TokenID separation_token, TokenID end_token)
    {
        List<ASTNode*> result = {};
        Token* token = nullptr;
        // TODO: messy
        while (((token = get_token(pc))->id) != end_token)
        {
            ASTNode* parameter = parse_parameter(pc);
            if (parameter)
            {
                result.append(parameter);
            }
            else
            {
                error(pc, get_token(pc), "Error parsing parameter: 0x%p\n", token);
            }
        }
        redassert(token->id == end_token)
        consume_token(pc);

        return result;
    }

    static inline ASTNode* parse_return_type(ParseContext* pc)
    {
        Token* token = get_token(pc);
        redassert(token);
        switch (token->id)
        {
            case TOKEN_ID_SEMICOLON:
            {
                ASTNode* node = create_node(pc, NODE_TYPE_TYPE, token);
                node->type_expr.is_void = true;
                node->type_expr.type = token_void_buffer();
                return node;
            }
            case TOKEN_ID_SYMBOL:
                consume_token(pc);
                return token_type_symbol(pc, token);
        }
        RED_NOT_IMPLEMENTED;
        return nullptr;
    }

    static inline ASTNode* parse_function_prototype(ParseContext* pc)
    {
        Token* symbol_token = get_token(pc);
        if (symbol_token->id != TOKEN_ID_SYMBOL)
        {
            return nullptr;
        }
        Token* equal_sign = get_token_i(pc, 1);
        if (equal_sign->id != TOKEN_ID_EQ)
        {
            return nullptr;
        }
        Token* left_parenthesis = get_token_i(pc, 2);
        if (left_parenthesis->id != TOKEN_ID_LEFT_PARENTHESIS)
        {
            return nullptr;
        }
        expect_and_consume_token(pc, TOKEN_ID_SYMBOL);
        expect_and_consume_token(pc, TOKEN_ID_EQ);
        expect_and_consume_token(pc, TOKEN_ID_LEFT_PARENTHESIS);

        ASTNode* result = create_node(pc, NODE_TYPE_FN_PROTO, symbol_token);
        result->fn_prototype.name = token_buffer(symbol_token);
        result->fn_prototype.parameters = parse_parameter_declaration_list(pc, TOKEN_ID_COMMA, TOKEN_ID_RIGHT_PARENTHESIS);
        result->fn_prototype.return_type = parse_return_type(pc);
        result->fn_prototype.external_linkage = false;

        return result;
    }

    static inline ASTNode* parse_return_expression(ParseContext* pc)
    {
        Token* first = consume_token_if(pc, TOKEN_ID_KEYWORD_RETURN);
        if (!first)
        {
            return nullptr;
        }
        ASTNode* return_value = parse_right_value(pc);
        if (!return_value)
        {
            return nullptr;
        }
        expect_token(pc, TOKEN_ID_SEMICOLON);
        consume_token(pc);
        ASTNode* return_expr = create_node(pc, NODE_TYPE_RETURN_EXPR, first);
        return_expr->return_expr.expression = return_value;

        return return_expr;
    }

    static inline ASTNode* parse_function_call(ParseContext* pc)
    {
        Token* token = get_token(pc);
        if (token->id == TOKEN_ID_SYMBOL && get_token_i(pc, 1)->id == TOKEN_ID_LEFT_PARENTHESIS)
        {
            consume_token(pc);
            consume_token(pc);
            
            ASTNode* fn_call_node = create_node(pc, NODE_TYPE_FN_CALL_EXPR, token);
            fn_call_node->fn_call_expr.name = token_buffer(token);
            fn_call_node->fn_call_expr.scope_function = nullptr;
            fn_call_node->fn_call_expr.parameters = parse_parameter_list(pc, TOKEN_ID_COMMA, TOKEN_ID_RIGHT_PARENTHESIS);
            redassert(get_token(pc)->id == TOKEN_ID_SEMICOLON);
            consume_token(pc);

            return fn_call_node;
        }
        return nullptr;
    }
    static inline ASTNode* parse_statement(ParseContext* pc)
    {
        ASTNode* fn_call_expr = parse_function_call(pc);
        if (fn_call_expr)
        {
            return fn_call_expr;
        }

        ASTNode* return_expr = parse_return_expression(pc);
        if (return_expr)
        {
            return return_expr;
        }

        return nullptr;
    }

    static inline ASTNode* parse_function_body(ParseContext* pc)
    {
        Token* first = expect_token(pc, TOKEN_ID_LEFT_BRACE);
        consume_token(pc);

        List<ASTNode*> statements = {};
        ASTNode* statement;
        while ((statement = parse_statement(pc)) != nullptr)
        {
            statements.append(statement);
        }

        expect_token(pc, TOKEN_ID_RIGHT_BRACE);
        consume_token(pc);

        ASTNode* fn_body = create_node(pc, NODE_TYPE_BLOCK, first);
        fn_body->block.statements = statements;

        return fn_body;
    }

    static inline ASTNode* parse_extern_function(ParseContext* pc)
    {
        Token* first = consume_token_if(pc, TOKEN_ID_KEYWORD_EXTERN);
        if (first)
        {
            ASTNode* fn_proto = parse_function_prototype(pc);
            if (fn_proto)
            {
                expect_and_consume_token(pc, TOKEN_ID_SEMICOLON);
                fn_proto->fn_prototype.external_linkage = true;
                return fn_proto;
            }
        }
        return nullptr;
    }

    static inline ASTNode* parse_function_declaration(ParseContext* pc)
    {
        //@unused
        //Token* first = get_token(pc);
        ASTNode* fn_prototype = parse_function_prototype(pc);
        if (!fn_prototype)
        {
            return nullptr;
        }
        ASTNode* fn_body = parse_function_body(pc);
        if (!fn_body)
        {
            return nullptr;
        }

        ASTNode* fn_declaration = create_node_copy_line_info(pc, NODE_TYPE_FN_DEF, fn_prototype);
        fn_prototype->fn_prototype.function_definition_node = fn_declaration;
        fn_declaration->fn_definition.function_prototype = fn_prototype;
        fn_declaration->fn_definition.body = fn_body;
        
        return fn_declaration;
    }

    static inline ASTNode* parse_top_level_declaration(ParseContext* pc)
    {
        Token* token = get_token(pc);
        if (is_container_keyword(token->id))
        {
#if PARSER_VERBOSE
            print("[TOKEN PARSE END] Parsed container declaration\n");
#endif
            return parse_container_declaration(pc);
        }
        else if (is_variable_keyword(token->id))
        {
#if PARSER_VERBOSE
            print("[TOKEN PARSE END] Parsed variable declaration\n");
#endif
            return parse_variable_declaration(pc);
        }
        else
        {
            ASTNode* symbol_node = parse_struct_union_member(pc);
            if (symbol_node)
            {
#if PARSER_VERBOSE
                print("[TOKEN PARSE END] Parsed container field\n");
#endif
                return symbol_node;
            }

            symbol_node = parse_extern_function(pc);
            if (symbol_node)
            {
#if PARSER_VERBOSE
                print("[TOKEN PARSE END] Parsed extern function prototype\n");
#endif
                return symbol_node;
            }
            
            symbol_node = parse_function_declaration(pc);
            if (symbol_node)
            {
#if PARSER_VERBOSE
                print("[TOKEN PARSE END] Parsed function declaration\n");
#endif
                return symbol_node;
            }

            symbol_node = parse_symbol_assignment(pc);
            if (symbol_node)
            {
#if PARSER_VERBOSE
                print("[TOKEN PARSE END] Parsed symbol assignment\n");
#endif
                return symbol_node;
            }

#if PARSER_VERBOSE
            print("[TOKEN PARSE END] Failed to parse!\n");
#endif
            return symbol_node;
        }

        return nullptr;
    }

    static ASTNodeContainerDeclaration parse_container_members(ParseContext* pc)
    {
        ASTNodeContainerDeclaration result = {};

        while (true)
        {
            Token* token = get_token(pc);
            if (token)
            {
                if (token->id == TOKEN_ID_RIGHT_BRACE)
                {
                    consume_token(pc);
                    break;
                }
                if (token->id == TOKEN_ID_END_OF_FILE)
                {
                    break;
                }
#if PARSER_VERBOSE
                PRINT_TOKEN_WITH_PREFIX("[TOKEN PARSE START] Trying to parse", token, symbol_name);
#endif
            }
            ASTNode* top_level_declaration = parse_top_level_declaration(pc);
            if (top_level_declaration)
            {
                result.declarations.append(top_level_declaration);
            }
            else
            {
                if (pc->current_token == pc->tokens->length - 1)
                {
                    break;
                }
                RED_NOT_IMPLEMENTED;
            }
        }

        return result;
    }

    static inline ASTNode* parse_root(ParseContext* pc)
    {
        Token* first = get_token(pc);
        ASTNodeContainerDeclaration root_members = parse_container_members(pc);
        if (pc->current_token != pc->tokens->length - 1)
        {
            invalid_token_error(pc, get_token(pc));
        }


        ASTNode* root_node_type = create_node(pc, NODE_TYPE_TYPE, first);
        root_node_type->type_expr.type = buf_alloc();
        buf_init_from_str(root_node_type->type_expr.type, "ROOT");

        ASTNode* root_node = create_node(pc, NODE_TYPE_CONTAINER_DECL, first);
        root_node->container_decl.is_root = true;
        root_node->container_decl.declarations = root_members.declarations;
        root_node->container_decl.fields = root_members.fields;
        root_node->container_decl.type_node = root_node_type;

        return root_node;
    }
}

ASTNode* red_parse(Buffer*buffer, List<Token>*tokens, RedType*owner)
{
    ParseContext pc = {};
    pc.tokens = tokens;
    pc.buffer = buffer;
    pc.owner = owner;
    return RedAST::parse_root(&pc);
}
#endif
