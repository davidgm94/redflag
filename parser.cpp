//
// Created by david on 10/11/20.
//

#include "parser.h"
#include "error_message.h"
#include <stdarg.h>

struct ParseContext
{
    Buffer* buffer;
    size_t current_token;
    List<Token>* tokens;
    RedType* owner;
    ErrorColor error_color;
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

static ASTNode* AST_parse_error_union_expression(ParseContext* pc);
static ASTNode* AST_parse_array_type_start(ParseContext* pc);
static ASTNode* AST_parse_prefix_type_op(ParseContext* pc);
static ASTNode* AST_parse_ptr_type_start(ParseContext* pc);
static ASTNode* AST_parse_type_expression(ParseContext* pc);
static ASTNode* AST_parse_expression(ParseContext* pc);
static ASTNode* AST_parse_test_declaration(ParseContext* pc);
static ASTNode* AST_parse_top_level_compile_time(ParseContext* pc);
static ASTNode* AST_parse_top_level_declaration(ParseContext* pc, Visibility visibility, Buffer*pList);
static ASTNode* AST_parse_container_field(ParseContext* pc);
static ASTNode* AST_parse_byte_align(ParseContext* pc);
static ASTNode* AST_parse_ptr_type_start(ParseContext* pc);
static ASTNode* AST_parse_array_type_start(ParseContext* pc);
static ASTNode* AST_parse_suffix_expression(ParseContext* pc);
static ASTNode* AST_parse_primary_type_expression(ParseContext* pc);
static ASTNode* AST_parse_suffix_op(ParseContext* pc);
static ASTNode* AST_parse_function_call_arguments(ParseContext* pc);
static ASTNode* AST_parse_block_expression_statement(ParseContext* pc);
static ASTNode* AST_parse_link_section(ParseContext*pc);
static ASTNode* AST_parse_if_statement(ParseContext*pc);
static ASTNode* AST_parse_assign_expression(ParseContext*pc);
static ASTNode* AST_parse_labeled_statement(ParseContext*pc);
static ASTNode* AST_parse_switch_expression(ParseContext*pc);
static ASTNode* AST_parse_function_prototype(ParseContext* pc);
static ASTNode* AST_parse_parameter_declaration(ParseContext* pc);
static Token* AST_parse_payload(ParseContext*pc);

ASTNode*AST_parse_call_convention_expression(ParseContext*pc);

static ASTNode*AST_parse_block(ParseContext*pc);

static void AST_error(ParseContext* pc, Token* token, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    Buffer* message = buf_vprintf(format, args);
    va_end(args);

    //ErrorMessage* error =
    RED_NOT_IMPLEMENTED;
}

static void AST_invalid_token_error(ParseContext* pc, Token* token)
{
    AST_error(pc, token, "invalid token: '%s'", token_name(token->id));
}

static inline ASTNode* AST_create_node_no_line_info(ParseContext* pc, NodeType type)
{
    ASTNode* node = new_elements(ASTNode, 1);
    node->type = type;
    node->owner = pc->owner;
    return node;
}

static ASTNode* AST_create_node(ParseContext* pc, NodeType type, Token* first_token)
{
    assert(first_token);
    ASTNode* node = AST_create_node_no_line_info(pc, type);
    node->line = first_token->start_line;
    node->column = first_token->start_column;
    return node;
}

static ASTNode* AST_create_node_copy_line_info(ParseContext* pc, NodeType type, ASTNode* from)
{
    assert(from);
    ASTNode* node = AST_create_node_no_line_info(pc, type);
    node->line = from->line;
    node->column = from->column;
    return node;
}

static inline Token* peek_token_i(ParseContext* pc, size_t i)
{
    return &pc->tokens->at(pc->current_token + i);
}

static inline Token* peek_token(ParseContext* pc)
{
    return peek_token_i(pc, 0);
}

static inline Token* eat_token(ParseContext* pc)
{
    Token* eaten = peek_token(pc);
    pc->current_token += 1;
    return eaten;
}

static Token* eat_token_if(ParseContext* pc, TokenID id)
{
    Token* eaten = peek_token(pc);
    if (eaten->id == id)
    {
        return eat_token(pc);
    }
    return nullptr;
}

static Token* expect_token(ParseContext* pc, TokenID id)
{
    Token* token = peek_token(pc);
    if (token->id != id)
    {
        AST_error(pc, token, "Expected token '%s', found '%s'", token_name(id), token_name(token->id));
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
    ASTNode* node = AST_create_node(pc, NODE_TYPE_SYMBOL, token);
    node->data.symbol_expr.symbol = token_buffer(token);
    return node;
}

template <typename T>
static List<T*> AST_parse_list(ParseContext* pc, TokenID sep, T* (*parser)(ParseContext*))
{
    List<T*> list = {};
    while (true)
    {
        T* current = parser(pc);
        if (current == nullptr)
        {
            break;
        }

        list.append(current);
        if (eat_token_if(pc, sep) == nullptr)
        {
            break;
        }
    }
    return list;
}

static ASTNode* AST_expect(ParseContext* pc, ASTNode* (*parser)(ParseContext*))
{
    ASTNode* result = parser(pc);
    if (result == nullptr)
    {
        AST_invalid_token_error(pc, peek_token(pc));
    }

    return result;
}

template <TokenID id, BinaryOpType op>
ASTNode* AST_parse_binary_op_simple(ParseContext* pc)
{
    Token* op_token = eat_token_if(pc, id);
    if (!op_token)
    {
        return nullptr;
    }

    ASTNode* result = AST_create_node(pc, NODE_TYPE_BIN_OP_EXPR, op_token);
    result->data.bin_op_expr.op = op;
    return result;
}

enum BinaryOpChain
{
    BINARY_OP_CHAIN_ONCE,
    BINARY_OP_CHAIN_INF,
};

static ASTNode* AST_parse_prefix_op_expression(ParseContext* pc,
                                               ASTNode* (*op_parser)(ParseContext*),
                                               ASTNode* (*child_parser)(ParseContext*))
{
    ASTNode* result = nullptr;
    ASTNode** right = &result;

    while (true)
    {
        ASTNode* prefix = op_parser(pc);

        if (prefix == nullptr)
        {
            break;
        }

        *right = prefix;

        switch (prefix->type)
        {
            case NODE_TYPE_PREFIX_OP_EXPR:
                right = &prefix->data.prefix_op_expr.expression;
                break;
            case NODE_TYPE_RETURN_EXPR:
                right = &prefix->data.return_expr.expression;
                break;
            case NODE_TYPE_AWAIT_EXPR:
                right = &prefix->data.await_expr.expression;
                break;
            case NODE_TYPE_ANY_FRAME_TYPE:
                right = &prefix->data.anyframe_type.payload_type;
                break;
            case NODE_TYPE_ARRAY_TYPE:
                right = &prefix->data.array_type.child_type;
                break;
            case NODE_TYPE_POINTER_TYPE:
            {
                ASTNode*child = prefix->data.pointer_type.op_expr;
                if (child == nullptr)
                {
                    child = prefix;
                }
                right = &child->data.pointer_type.op_expr;
                break;
            }
            default:
                RED_UNREACHABLE;
        }
    }

    if (result != nullptr)
    {
        *right = AST_expect(pc, child_parser);
    }
    else
    {
        *right = child_parser(pc);
        if (*right == nullptr)
        {
            return nullptr;
        }
    }

    return result;
}

static ASTNode* AST_parse_binary_op_expression(ParseContext* pc, BinaryOpChain chain,
                                               ASTNode* (*op_parse) (ParseContext*),
                                               ASTNode* (*child_parse)(ParseContext*))
{
    ASTNode* node = child_parse(pc);
    if (node == nullptr)
    {
        return nullptr;
    }

    do
    {
        ASTNode* op = op_parse(pc);
        if (op == nullptr)
        {
            break;
        }

        ASTNode* left = node;
        ASTNode* right = AST_expect(pc, child_parse);
        node = op;

        switch (op->type)
        {
            case NODE_TYPE_BIN_OP_EXPR:
                op->data.bin_op_expr.op1 = left;
                op->data.bin_op_expr.op2 = right;
                break;
            default:
                RED_UNREACHABLE;
        }
    } while (chain == BINARY_OP_CHAIN_INF);

    return node;
}


//static ASTNode* AST_parse_if_prefix(ParseContext* pc)
//{
//    Token* first = eat_token_if(pc, TOKEN_ID_KEYWORD_IF);
//    if (first == nullptr)
//    {
//        return nullptr;
//    }
//
//    expect_token(pc, TOKEN_ID_LEFT_PARENTHESIS);
//    ASTNode* condition = AST_expect(pc, ast_parse_expr)
//}

//static ASTNode* AST_parse_if_expression_helper(ParseContext* pc,
//                                               ASTNode* *(body_parser)(ParseContext*))
//{
//    ASTNode* node = AST_parse_if_pre
//}

enum ContainerFieldState
{
    CONTAINER_FIELD_STATE_NONE,
    CONTAINER_FIELD_STATE_SEEN,
    CONTAINER_FIELD_STATE_END,
};

static ASTNodeContainerDeclaration AST_parse_container_members(ParseContext* pc)
{
    ASTNodeContainerDeclaration result = {};
    ContainerFieldState field_state = CONTAINER_FIELD_STATE_NONE;
    Token* first_token = nullptr;

    for (;;)
    {
        Token* peeked_token = peek_token(pc);
        ASTNode* test_declaration = AST_parse_test_declaration(pc);
        if (test_declaration != nullptr)
        {
            if (field_state == CONTAINER_FIELD_STATE_SEEN)
            {
                field_state = CONTAINER_FIELD_STATE_END;
                first_token = peeked_token;
            }
            result.declarations.append(test_declaration);
            continue;
        }

        ASTNode* top_level_compile_time = AST_parse_top_level_compile_time(pc);
        if (top_level_compile_time != nullptr)
        {
            if (field_state == CONTAINER_FIELD_STATE_SEEN)
            {
                field_state = CONTAINER_FIELD_STATE_END;
                first_token = peeked_token;
            }
            result.declarations.append(top_level_compile_time);
            continue;
        }

        Buffer doc_buffer;
        // TODO: doc
        peeked_token = peek_token(pc);

        Token* visibility_token = eat_token_if(pc, TOKEN_ID_KEYWORD_PUB);
        Visibility visibility = visibility_token != nullptr ? VISIBILITY_PUB : VISIBILITY_PRIVATE;

        ASTNode* top_level_declaration = AST_parse_top_level_declaration(pc, visibility, &doc_buffer);
        if (top_level_declaration != nullptr)
        {
            if (field_state == CONTAINER_FIELD_STATE_SEEN)
            {
                field_state = CONTAINER_FIELD_STATE_END;
                first_token = peeked_token;
            }
            result.declarations.append(top_level_declaration);
            continue;
        }

        if (visibility_token != nullptr)
        {
            AST_error(pc, peek_token(pc), "Expected function or variable declaration after keyword 'pub'");
        }

        Token* compile_time_token = eat_token_if(pc, TOKEN_ID_KEYWORD_COMPTIME);

        ASTNode* container_field = AST_parse_container_field(pc);
        if (container_field != nullptr)
        {
            switch (field_state)
            {
                case CONTAINER_FIELD_STATE_NONE:
                    field_state = CONTAINER_FIELD_STATE_SEEN;
                    break;
                case CONTAINER_FIELD_STATE_SEEN:
                    break;
                case CONTAINER_FIELD_STATE_END:
                    AST_error(pc, first_token, "Declarations are not allowed between container fields");
            }

            assert(container_field->type == NODE_TYPE_STRUCT_FIELD);
            container_field->data.struct_field.document_comments = doc_buffer;
            container_field->data.struct_field.compile_time_token = compile_time_token;
            result.fields.append(container_field);

            if (eat_token_if(pc, TOKEN_ID_COMMA) != nullptr)
            {
                continue;
            }
            else
            {
                break;
            }
        }
        break;
    }

    //result.document_comments =
    return result;
}

static ASTNode* AST_parse_container_field(ParseContext* pc)
{
    Token* identifier = eat_token_if(pc, TOKEN_ID_SYMBOL);
    if (identifier == nullptr)
    {
        return nullptr;
    }

    ASTNode* type_expression = nullptr;
    if (eat_token_if(pc, TOKEN_ID_COLON) != nullptr)
    {
        Token* any_type_token = eat_token_if(pc, TOKEN_ID_KEYWORD_ANY);
        if (any_type_token != nullptr)
        {
            type_expression = AST_create_node(pc, NODE_TYPE_ANY_TYPE_FIELD, any_type_token);
        }
        else
        {
            type_expression = AST_expect(pc, AST_parse_expression);
        }
    }

    ASTNode* align_expression = AST_parse_byte_align(pc);
    ASTNode* expression = nullptr;
    if (eat_token_if(pc, TOKEN_ID_EQ) != nullptr)
    {
        expression = AST_expect(pc, AST_parse_expression);
    }

    ASTNode* result = AST_create_node(pc, NODE_TYPE_STRUCT_FIELD, identifier);
    result->data.struct_field.name = token_buffer(identifier);
    result->data.struct_field.type = type_expression;
    result->data.struct_field.value = expression;
    result->data.struct_field.align_expression = align_expression;

    return result;
}



static ASTNode* AST_parse_variable_declaration(ParseContext* pc)
{
    Token* mut_keyword = eat_token_if(pc, TOKEN_ID_KEYWORD_CONST);
    if (mut_keyword == nullptr)
    {
        mut_keyword = eat_token_if(pc, TOKEN_ID_KEYWORD_CONST);
    }

    if (mut_keyword == nullptr)
    {
        return nullptr;
    }

    Token* identifier = expect_token(pc, TOKEN_ID_SYMBOL);
    ASTNode* type_expression = nullptr;
    if (eat_token_if(pc, TOKEN_ID_COLON) != nullptr)
    {
        type_expression = AST_expect(pc, AST_parse_type_expression);
    }

    ASTNode* align_expression = AST_parse_byte_align(pc);
    ASTNode* section_expression = AST_parse_link_section(pc);

    ASTNode* expression = nullptr;
    if (eat_token_if(pc, TOKEN_ID_EQ) != nullptr)
    {
        expression = AST_expect(pc, AST_parse_expression);
    }

    expect_token(pc, TOKEN_ID_SEMICOLON);

    ASTNode* result = AST_create_node(pc, NODE_TYPE_VARIABLE_DECLARATION, mut_keyword);
    result->data.variable_declaration.is_const = mut_keyword->id == TOKEN_ID_KEYWORD_CONST;
    result->data.variable_declaration.symbol = token_buffer(identifier);
    result->data.variable_declaration.type = type_expression;
    result->data.variable_declaration.align_expression = align_expression;
    result->data.variable_declaration.section_expression = section_expression;
    result->data.variable_declaration.expression = expression;

    return result;
}


static ASTNode* AST_parse_statement(ParseContext* pc)
{
    Token* comp_time = eat_token_if(pc, TOKEN_ID_KEYWORD_COMPTIME);
    ASTNode* var_decl = AST_parse_variable_declaration(pc);
    if (var_decl)
    {
        assert(var_decl->type == NODE_TYPE_VARIABLE_DECLARATION);
        var_decl->data.variable_declaration.is_compile_time = comp_time != nullptr;
    }

    if (comp_time != nullptr)
    {
        ASTNode* statement = AST_expect(pc, AST_parse_block_expression_statement);
        ASTNode* result = AST_create_node(pc, NODE_TYPE_COMP_TIME, comp_time);
        result->data.comptime_expr.expression = statement;
        return result;
    }

    Token* defer = eat_token_if(pc, TOKEN_ID_KEYWORD_DEFER);
    if (!defer)
    {
        defer = eat_token_if(pc, TOKEN_ID_KEYWORD_ERROR_DEFER);
    }
    if (defer)
    {
        Token* payload = (defer->id == TOKEN_ID_KEYWORD_ERROR_DEFER) ? AST_parse_payload(pc) : nullptr;
        ASTNode* statement = AST_expect(pc, AST_parse_block_expression_statement);
        ASTNode* result = AST_create_node(pc, NODE_TYPE_DEFER, defer);

        result->data.defer.type = RETURN_TYPE_UNCONDITIONAL;
        result->data.defer.expression = statement;

        if (defer->id == TOKEN_ID_KEYWORD_ERROR_DEFER)
        {
            result->data.defer.type = RETURN_TYPE_ERROR;
            if (payload)
            {
                result->data.defer.error_payload = token_symbol(pc, payload);
            }
        }
        return result;
    }

    ASTNode* if_statement = AST_parse_if_statement(pc);
    if (if_statement)
    {
        return if_statement;
    }

    ASTNode* labeled_statement = AST_parse_labeled_statement(pc);
    if (labeled_statement)
    {
        return labeled_statement;
    }

    ASTNode* switch_expression = AST_parse_switch_expression(pc);
    if (switch_expression)
    {
        return switch_expression;
    }

    ASTNode* assign = AST_parse_assign_expression(pc);
    if (assign)
    {
        expect_token(pc, TOKEN_ID_SEMICOLON);
        return assign;
    }

    return nullptr;
}

static inline ASTNode* AST_parse_expression_inside_parenthesis_helper(ParseContext* pc)
{
    expect_token(pc, TOKEN_ID_LEFT_PARENTHESIS);
    ASTNode* result = AST_expect(pc, AST_parse_expression);
    expect_token(pc, TOKEN_ID_RIGHT_PARENTHESIS);

    return result;
}

static ASTNode* AST_parse_link_section(ParseContext*pc)
{
    Token* first = eat_token_if(pc, TOKEN_ID_KEYWORD_SECTION);
    if (!first)
    {
        return nullptr;
    }

    return AST_parse_expression_inside_parenthesis_helper(pc);
}

static ASTNode*AST_parse_byte_align(ParseContext* pc)
{
    if (!eat_token_if(pc, TOKEN_ID_KEYWORD_ALIGN))
    {
        return nullptr;
    }

    return AST_parse_expression_inside_parenthesis_helper(pc);
}

static ASTNode* AST_parse_top_level_compile_time(ParseContext* pc)
{
    Token* compile_time = eat_token_if(pc, TOKEN_ID_KEYWORD_COMPTIME);
    if (!compile_time)
    {
        return nullptr;
    }

    Token* left_brace = peek_token(pc);
    if (left_brace->id != TOKEN_ID_LEFT_BRACE)
    {
        put_back_token(pc);
        return nullptr;
    }

    ASTNode* block = AST_expect(pc, AST_parse_block_expression_statement);
    ASTNode* result = AST_create_node(pc, NODE_TYPE_COMP_TIME, compile_time);
    result->data.comptime_expr.expression = block;

    return result;
}

static ASTNode* AST_parse_top_level_declaration(ParseContext* pc, Visibility visibility, Buffer* doc_comments)
{
    Token *first = eat_token_if(pc, TOKEN_ID_KEYWORD_EXPORT);
    if (first == nullptr)
        first = eat_token_if(pc, TOKEN_ID_KEYWORD_EXTERN);
    if (first == nullptr)
        first = eat_token_if(pc, TOKEN_ID_KEYWORD_INLINE);
    if (first == nullptr)
        first = eat_token_if(pc, TOKEN_ID_KEYWORD_NO_INLINE);
    if (first != nullptr)
    {
        Token *lib_name = nullptr;
        if (first->id == TOKEN_ID_KEYWORD_EXTERN)
            lib_name = eat_token_if(pc, TOKEN_ID_STRING_LIT);

        if (first->id != TOKEN_ID_KEYWORD_INLINE && first->id != TOKEN_ID_KEYWORD_NO_INLINE) {
            Token *thread_local_kw = eat_token_if(pc, TOKEN_ID_KEYWORD_THREAD_LOCAL);
            ASTNode* var_decl = AST_parse_variable_declaration(pc);
            if (var_decl != nullptr)
            {
                assert(var_decl->type == NODE_TYPE_VARIABLE_DECLARATION);
                if (first->id == TOKEN_ID_KEYWORD_EXTERN && var_decl->data.variable_declaration.expression != nullptr)
                {
                    AST_error(pc, first, "extern variables have no initializers");
                }
                var_decl->line = first->start_line;
                var_decl->column = first->start_column;
                var_decl->data.variable_declaration.threadlocal_token = thread_local_kw;
                var_decl->data.variable_declaration.visibility = visibility;
                var_decl->data.variable_declaration.document_comments = *doc_comments;
                var_decl->data.variable_declaration.is_extern = first->id == TOKEN_ID_KEYWORD_EXTERN;
                var_decl->data.variable_declaration.is_export = first->id == TOKEN_ID_KEYWORD_EXPORT;
                var_decl->data.variable_declaration.library_name = token_buffer(lib_name);
                return var_decl;
            }

            if (thread_local_kw != nullptr)
                put_back_token(pc);
        }

        ASTNode *fn_prototype = AST_parse_function_prototype(pc);
        if (fn_prototype != nullptr) {
            ASTNode* body = AST_parse_block(pc);
            if (body == nullptr)
                expect_token(pc, TOKEN_ID_SEMICOLON);

            assert(fn_prototype->type == NODE_TYPE_FN_PROTO);
            fn_prototype->line = first->start_line;
            fn_prototype->column = first->start_column;
            fn_prototype->data.fn_prototype.visibility = visibility;
            fn_prototype->data.fn_prototype.documentation_comments = *doc_comments;
            if (!fn_prototype->data.fn_prototype.is_extern)
                fn_prototype->data.fn_prototype.is_extern = first->id == TOKEN_ID_KEYWORD_EXTERN;
            fn_prototype->data.fn_prototype.is_export = first->id == TOKEN_ID_KEYWORD_EXPORT;
            switch (first->id) {
                case TOKEN_ID_KEYWORD_INLINE:
                    fn_prototype->data.fn_prototype.function_inline = FUNCTION_INLINE_ALWAYS;
                    break;
                case TOKEN_ID_KEYWORD_NO_INLINE:
                    fn_prototype->data.fn_prototype.function_inline = FUNCTION_INLINE_NEVER;
                    break;
                default:
                    fn_prototype->data.fn_prototype.function_inline = FUNCTION_INLINE_AUTO;
                    break;
            }
            fn_prototype->data.fn_prototype.library_name = token_buffer(lib_name);

            ASTNode *res = fn_prototype;
            if (body != nullptr) {
                if (fn_prototype->data.fn_prototype.is_extern) {
                    AST_error(pc, first, "extern functions have no body");
                }
                res = AST_create_node_copy_line_info(pc, NODE_TYPE_FN_DEF, fn_prototype);
                res->data.fn_definition.function_prototype = fn_prototype;
                res->data.fn_definition.body = body;
                fn_prototype->data.fn_prototype.function_definition_node = res;
            }

            return res;
        }

        AST_invalid_token_error(pc, peek_token(pc));
    }

    Token *thread_local_kw = eat_token_if(pc, TOKEN_ID_KEYWORD_THREAD_LOCAL);
    ASTNode *var_decl = AST_parse_variable_declaration(pc);
    if (var_decl != nullptr) {
        assert(var_decl->type == NODE_TYPE_VARIABLE_DECLARATION);
        var_decl->data.variable_declaration.visibility = visibility;
        var_decl->data.variable_declaration.document_comments = *doc_comments;
        var_decl->data.variable_declaration.threadlocal_token = thread_local_kw;
        return var_decl;
    }

    if (thread_local_kw != nullptr)
        put_back_token(pc);

    ASTNode *fn_prototype = AST_parse_function_prototype(pc);
    if (fn_prototype != nullptr) {
        ASTNode *body = AST_parse_block(pc);
        if (body == nullptr)
            expect_token(pc, TOKEN_ID_SEMICOLON);

        assert(fn_prototype->type == NODE_TYPE_FN_PROTO);
        fn_prototype->data.fn_prototype.visibility = visibility;
        fn_prototype->data.fn_prototype.documentation_comments = *doc_comments;
        ASTNode *res = fn_prototype;
        if (body != nullptr)
        {
            res = AST_create_node_copy_line_info(pc, NODE_TYPE_FN_DEF, fn_prototype);
            res->data.fn_definition.function_prototype = fn_prototype;
            res->data.fn_definition.body = body;
            fn_prototype->data.fn_prototype.function_definition_node = res;
        }

        return res;
    }

    return nullptr;
}

static ASTNode* AST_parse_block(ParseContext*pc)
{
    Token* left_brace = eat_token_if(pc, TOKEN_ID_LEFT_BRACE);
    if (!left_brace)
    {
        return nullptr;
    }

    List<ASTNode*> statements = {};
    ASTNode* statement;
    while ((statement = AST_parse_statement(pc)) != nullptr)
    {
        statements.append(statement);
    }
    expect_token(pc, TOKEN_ID_RIGHT_BRACE);

    ASTNode* result = AST_create_node(pc, NODE_TYPE_BLOCK, left_brace);
    result->data.block.statements = statements;

    return result;
}

static ASTNode*AST_parse_switch_expression(ParseContext*pc)
{
    return nullptr;
}

static ASTNode*AST_parse_labeled_statement(ParseContext*pc)
{
    return nullptr;
}

static ASTNode*AST_parse_assign_expression(ParseContext*pc)
{
    return nullptr;
}

static ASTNode*AST_parse_if_statement(ParseContext*pc)
{
    return nullptr;
}

static Token* AST_parse_payload(ParseContext*pc)
{
    return nullptr;
}
static ASTNode* AST_parse_expression(ParseContext* pc)
{
    // TODO: DELETE TRY
    return nullptr;
}
ASTNode*AST_parse_function_call_arguments(ParseContext* pc)
{
    return nullptr;
}

ASTNode*AST_parse_suffix_op(ParseContext* pc)
{
    return nullptr;
}

ASTNode*AST_parse_primary_type_expression(ParseContext* pc)
{
    return nullptr;
}

ASTNode*AST_parse_block_expression_statement(ParseContext*pc)
{
    return nullptr;
}

ASTNode* AST_parse_type_expression(ParseContext* pc)
{
    return AST_parse_prefix_op_expression(pc, AST_parse_prefix_type_op, AST_parse_error_union_expression);
}


static ASTNode* AST_parse_block(ParseContext* pc)
{
    Token* left_brace = eat_token_if(pc, TOKEN_ID_LEFT_BRACE);
    if (left_brace == nullptr)
    {
        return nullptr;
    }

    List<ASTNode*> statements ={};
    ASTNode* statement;

    while ((statement = AST_parse_statement(pc)) != nullptr)
    {
        statements.append(statement);
    }

    expect_token(pc, TOKEN_ID_RIGHT_BRACE);

    ASTNode* result = AST_create_node(pc, NODE_TYPE_BLOCK, left_brace);
    result->data.block.statements = statements;
    return result;
}

static ASTNode* AST_parse_test_declaration(ParseContext* pc)
{
    Token* test = eat_token_if(pc, TOKEN_ID_KEYWORD_TEST);
    if (test == nullptr)
    {
        return nullptr;
    }

    Token* name = expect_token(pc, TOKEN_ID_STRING_LIT);
    ASTNode* block = AST_expect(pc, AST_parse_block);
    ASTNode* result = AST_create_node(pc, NODE_TYPE_TEST_DECL, test);
    result->data.test_decl.name = token_buffer(name);
    result->data.test_decl.body = block;

    return result;
}

static ASTNode* AST_parse_root(ParseContext* pc)
{
    Token* first = peek_token(pc);
    ASTNodeContainerDeclaration members = AST_parse_container_members(pc);
    if (pc->current_token != pc->tokens->length - 1)
    {
        AST_invalid_token_error(pc, peek_token(pc));
    }

    ASTNode* node = AST_create_node(pc, NODE_TYPE_CONTAINER_DECL, first);
    node->data.container_decl.fields = members.fields;
    node->data.container_decl.declarations = members.declarations;
    node->data.container_decl.layout = CONTAINER_LAYOUT_AUTO;
    node->data.container_decl.type = CONTAINER_TYPE_STRUCT;
    node->data.container_decl.is_root = true;
    if (buf_len(&members.document_comments) != 0)
    {
        node->data.container_decl.document_comments = members.document_comments;
    }

    return node;
}

ASTNode* AST_parse_prefix_type_op(ParseContext* pc)
{
    Token* question_mark = eat_token_if(pc, TOKEN_ID_QUESTION);
    if (question_mark != nullptr)
    {
        ASTNode* result = AST_create_node(pc, NODE_TYPE_PREFIX_OP_EXPR, question_mark);
        result->data.prefix_op_expr.op = Prefix_Op_Optional;
        return result;
    }

    Token* any_frame = eat_token_if(pc, TOKEN_ID_KEYWORD_ANY_FRAME);
    if (any_frame != nullptr)
    {
        if (eat_token_if(pc, TOKEN_ID_ARROW) != nullptr)
        {
            ASTNode* result = AST_create_node(pc, NODE_TYPE_ANY_FRAME_TYPE, any_frame);
            return result;
        }
        put_back_token(pc);
    }

    Token* array_init_left_bracket = eat_token_if(pc, TOKEN_ID_LEFT_BRACKET);
    if (array_init_left_bracket != nullptr)
    {
        Token* underscore = eat_token_if(pc, TOKEN_ID_SYMBOL);
        if (underscore == nullptr)
        {
            put_back_token(pc);
        }
        else if (!buf_eql_str(token_buffer(underscore), "_"))
        {
            put_back_token(pc);
            put_back_token(pc);
        }
        else
        {
            ASTNode* sentinel = nullptr;
            Token* colon = eat_token_if(pc, TOKEN_ID_COLON);
            if (colon != nullptr)
            {
                sentinel = AST_expect(pc, AST_parse_expression);
            }
            expect_token(pc, TOKEN_ID_RIGHT_BRACKET);
            ASTNode* node = AST_create_node(pc, NODE_TYPE_INFERRED_ARRAYTYPE, array_init_left_bracket);
            node->data.inferred_array_type.sentinel = sentinel;
            return node;
        }
    }

    ASTNode* ptr = AST_parse_ptr_type_start(pc);
    if (ptr)
    {
        RED_NOT_IMPLEMENTED;
    }

    ASTNode* array = AST_parse_array_type_start(pc);
    if (array)
    {
        assert(array->type == NODE_TYPE_ARRAY_TYPE);
        while (true)
        {
            Token* allow_zero_token = eat_token_if(pc, TOKEN_ID_KEYWORD_ALLOW_ZERO);
            if (allow_zero_token)
            {
                array->data.array_type.allow_zero_token = allow_zero_token;
                continue;
            }

            ASTNode* align_expression = AST_parse_byte_align(pc);
            if (align_expression)
            {
                array->data.array_type.align_expression = align_expression;
                continue;
            }

            if (eat_token_if(pc, TOKEN_ID_KEYWORD_CONST))
            {
                array->data.array_type.is_const = true;
                continue;
            }

            if (eat_token_if(pc, TOKEN_ID_KEYWORD_VOLATILE))
            {
                array->data.array_type.is_volatile = true;
            }
            break;
        }
        return array;
    }
    return nullptr;
}



ASTNode* AST_parse_ptr_type_start(ParseContext* pc)
{
    return nullptr;
    ///////////////////
    ASTNode* sentinel = nullptr;

    Token* asterisk = eat_token_if(pc, TOKEN_ID_STAR);
    if (asterisk)
    {
        Token* colon = eat_token_if(pc, TOKEN_ID_COLON);
        if (colon)
        {
            sentinel = AST_expect(pc, AST_parse_expression);
        }
        ASTNode* result = AST_create_node(pc, NODE_TYPE_POINTER_TYPE, asterisk);
        result->data.pointer_type.star_token = asterisk;
        result->data.pointer_type.sentinel = sentinel;
        return result;
    }

    RED_NOT_IMPLEMENTED;
    // TODO: fuck
    //    Token* asterisk2 = eat_token_if(pc, star)
    return {};
}

ASTNode* AST_parse_array_type_start(ParseContext* pc)
{
    Token* left_bracket = eat_token_if(pc, TOKEN_ID_LEFT_BRACKET);
    if (left_bracket == nullptr)
    {
        return nullptr;
    }

    ASTNode* size = AST_parse_expression(pc);
    ASTNode* sentinel = nullptr;
    Token* colon = eat_token_if(pc, TOKEN_ID_COLON);
    if (colon)
    {
        sentinel = AST_expect(pc, AST_parse_expression);
    }

    expect_token(pc, TOKEN_ID_RIGHT_BRACKET);
    ASTNode* result = AST_create_node(pc, NODE_TYPE_ARRAY_TYPE, left_bracket);
    result->data.array_type.size = size;
    result->data.array_type.sentinel = sentinel;

    return result;
}

ASTNode*AST_parse_error_union_expression(ParseContext*pc)
{
    ASTNode* result = AST_parse_suffix_expression(pc);
    if (!result)
    {
        return nullptr;
    }

    ASTNode* op = AST_parse_binary_op_simple<TOKEN_ID_BANG, BIN_OP_TYPE_ERROR_UNION>(pc);
    if (!op)
    {
        return nullptr;
    }

    ASTNode* right = AST_expect(pc, AST_parse_type_expression);
    assert(op->type == NODE_TYPE_BIN_OP_EXPR);
    op->data.bin_op_expr.op1 = result;
    op->data.bin_op_expr.op2 = right;

    return op;
}

ASTNode*AST_parse_suffix_expression(ParseContext* pc)
{
    // No async

    ASTNode* result = AST_parse_primary_type_expression(pc);
    if (!result)
    {
        return nullptr;
    }

    while (true)
    {
        ASTNode* suffix = AST_parse_suffix_op(pc);
        if (suffix)
        {
            switch (suffix->type)
            {
                case NODE_TYPE_SLICE_EXPR:
                    suffix->data.slice_expr.array = result;
                    break;
                case NODE_TYPE_ARRAY_ACCESS_EXPR:
                    suffix->data.array_access_expr.array = result;
                    break;
                case NODE_TYPE_FIELD_ACCESS_EXPR:
                    suffix->data.field_access_expr.struct_ref = result;
                    break;
                case NODE_TYPE_PTR_DEREF:
                    suffix->data.ptr_deref_expr.target = result;
                    break;
                default:
                    RED_UNREACHABLE;
            }
            result = suffix;
            continue;
        }

        ASTNode* call = AST_parse_function_call_arguments(pc);
        if (call)
        {
            assert(call->type == NODE_TYPE_FN_CALL_EXPR);
            call->data.fn_call_expr.function = result;
            result = call;
            continue;
        }
        break;
    }

    return result;
}

ASTNode* AST_parse_function_prototype(ParseContext* pc)
{
    Token* first = eat_token_if(pc, TOKEN_ID_KEYWORD_FN);
    if (!first)
    {
        return nullptr;
    }

    Token* identifier = eat_token_if(pc, TOKEN_ID_SYMBOL);
    expect_token(pc, TOKEN_ID_LEFT_PARENTHESIS);
    List<ASTNode*> params = AST_parse_list(pc, TOKEN_ID_COMMA, AST_parse_parameter_declaration);
    expect_token(pc, TOKEN_ID_RIGHT_PARENTHESIS);

    ASTNode* align_expression = AST_parse_byte_align(pc);
    ASTNode* section_expression = AST_parse_link_section(pc);
    ASTNode* call_convention_expression = AST_parse_call_convention_expression(pc);
    Token* any_type = eat_token_if(pc, TOKEN_ID_KEYWORD_ANY);
    Token* exmark = nullptr;
    ASTNode* return_type = nullptr;
    if (any_type == nullptr)
    {
        exmark = eat_token_if(pc, TOKEN_ID_BANG);
        return_type = AST_expect(pc, AST_parse_type_expression);
    }

}

ASTNode*AST_parse_call_convention_expression(ParseContext*pc)
{
    return nullptr;
}


ASTNode* parse(Buffer*buffer, List<Token>*tokens, RedType*owner, ErrorColor error_color)
{
    ParseContext pc = {};
    pc.buffer = buffer;
    pc.owner = owner;
    pc.error_color = error_color;
    pc.tokens = tokens;
    return AST_parse_root(&pc);
}
