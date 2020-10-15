//
// Created by david on 10/11/20.
//

#include "parser.h"
#include "error_message.h"
#include "optional.h"
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
static ASTNode* AST_parse_call_convention_expression(ParseContext*pc);
static ASTNode* AST_parse_block(ParseContext*pc);
static Token* AST_parse_payload(ParseContext*pc);

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

static ASTNode* AST_parse_switch_expression(ParseContext* pc)
{
    Token* switch_token = eat_token_if(pc, TOKEN_ID_KEYWORD_SWITCH);
    if (!switch_token)
    {
        return nullptr;
    }

    expect_token(pc, TOKEN_ID_LEFT_PARENTHESIS);
    ASTNode* expression = AST_expect(pc, AST_parse_expression);
    expect_token(pc, TOKEN_ID_RIGHT_PARENTHESIS);
    expect_token(pc, TOKEN_ID_LEFT_BRACE);
    List<ASTNode*> cases = AST_parse_list(pc, TOKEN_ID_COMMA, AST_parse_switch_expression);
    expect_token(pc, TOKEN_ID_RIGHT_BRACE);

    ASTNode* result = AST_create_node(pc, NODE_TYPE_SWITCH_EXPR, switch_token);
    result->data.switch_expr.expression = expression;
    result->data.switch_expr.cases = cases;
    return result;
}

static Token* AST_parse_block_label(ParseContext* pc)
{
    Token* ident = eat_token_if(pc, TOKEN_ID_SYMBOL);
    if (!ident)
    {
        return nullptr;
    }

    if (!eat_token_if(pc, TOKEN_ID_COLON))
    {
        put_back_token(pc);
        return nullptr;
    }

    return ident;
}

static Optional<PointerIndexPayload> AST_parse_ptr_index_payload(ParseContext* pc)
{
    if (!eat_token_if(pc, TOKEN_ID_BIT_OR))
    {
        return Optional<PointerIndexPayload>::none();
    }

    Token* asterisk = eat_token_if(pc, TOKEN_ID_STAR);
    Token* payload = expect_token(pc, TOKEN_ID_SYMBOL);
    Token* index = nullptr;
    if (eat_token_if(pc, TOKEN_ID_COMMA))
    {
        index = expect_token(pc, TOKEN_ID_SYMBOL);
    }
    expect_token(pc, TOKEN_ID_BIT_OR);

    PointerIndexPayload result;
    result.asterisk = asterisk;
    result.payload = payload;
    result.index = index;

    return Optional<PointerIndexPayload>::some(result);
}

static ASTNode* AST_parse_for_prefix(ParseContext* pc)
{
    Token* for_token = eat_token_if(pc, TOKEN_ID_KEYWORD_FOR);
    if (!for_token)
    {
        return nullptr;
    }

    expect_token(pc, TOKEN_ID_LEFT_PARENTHESIS);
    ASTNode* array_expression = AST_expect(pc, AST_parse_expression);
    expect_token(pc, TOKEN_ID_RIGHT_PARENTHESIS);

    PointerIndexPayload payload;
    if (!AST_parse_ptr_index_payload(pc).unwrap(&payload))
    {
        AST_invalid_token_error(pc, peek_token(pc));
    }

    ASTNode* result = AST_create_node(pc, NODE_TYPE_FOR_EXPR, for_token);
    result->data.for_expr.array_expression = array_expression;
    result->data.for_expr.element_node = token_symbol(pc, payload.payload);
    result->data.for_expr.element_is_ptr = payload.asterisk != nullptr;

    if (payload.index != nullptr)
    {
        result->data.for_expr.index_node = token_symbol(pc, payload.index);
    }

    return result;
}

static ASTNode* AST_parse_block_expression(ParseContext* pc)
{
    Token* label = AST_parse_block_label(pc);
    if (label)
    {
        ASTNode* result = AST_expect(pc, AST_parse_block);
        assert(result->type == NODE_TYPE_BLOCK);
        result->data.block.name = token_buffer(label);
        return result;
    }

    return AST_parse_block(pc);
}

static ASTNode* AST_parse_for_statement(ParseContext* pc)
{
    ASTNode* result = AST_parse_for_prefix(pc);
    if (!result)
    {
        return nullptr;
    }

    ASTNode* body = AST_parse_block_expression(pc);
    bool requires_semi = false;
    if (!body)
    {
        requires_semi = true;
        body = AST_parse_assign_expression(pc);
    }

    if (!body)
    {
        Token* token = eat_token(pc);
        AST_error(pc, token, "Expected loop body, found: '%s'", token_name(token->id));
    }

    ASTNode* else_body = nullptr;
    if (eat_token_if(pc, TOKEN_ID_KEYWORD_ELSE))
    {
        else_body = AST_expect(pc, AST_parse_statement);
    }

    if (requires_semi && !else_body)
    {
        expect_token(pc, TOKEN_ID_SEMICOLON);
    }

    assert(result->type == NODE_TYPE_FOR_EXPR);
    result->data.for_expr.body = body;
    result->data.for_expr.else_node = else_body;

    return result;
}

static Optional<PointerPayload> AST_parse_ptr_payload(ParseContext* pc)
{
    if (eat_token_if(pc, TOKEN_ID_BIT_OR) == nullptr)
    {
        return Optional<PointerPayload>::none();
    }

    Token* asterisk = eat_token_if(pc, TOKEN_ID_STAR);
    Token* payload = expect_token(pc, TOKEN_ID_SYMBOL);
    expect_token(pc, TOKEN_ID_BIT_OR);

    PointerPayload result;
    result.asterisk = asterisk;
    result.payload = payload;

    return Optional<PointerPayload>::some(result);
}

static ASTNode* AST_parse_while_continue_expression(ParseContext* pc)
{
    Token* first = eat_token_if(pc, TOKEN_ID_COLON);
    if (!first)
    {
        return nullptr;
    }

    expect_token(pc, TOKEN_ID_LEFT_PARENTHESIS);
    ASTNode* expression = AST_expect(pc, AST_parse_assign_expression);
    expect_token(pc, TOKEN_ID_RIGHT_PARENTHESIS);

    return expression;
}

static ASTNode* AST_parse_while_prefix(ParseContext* pc)
{
    Token* while_token = eat_token_if(pc, TOKEN_ID_KEYWORD_WHILE);
    if (!while_token)
    {
        return nullptr;
    }

    expect_token(pc, TOKEN_ID_LEFT_PARENTHESIS);
    ASTNode* condition = AST_expect(pc, AST_parse_expression);
    expect_token(pc, TOKEN_ID_RIGHT_PARENTHESIS);
    Optional<PointerPayload> opt_payload = AST_parse_ptr_payload(pc);
    ASTNode* continue_expression = AST_parse_while_continue_expression(pc);

    PointerPayload payload;
    ASTNode* result = AST_create_node(pc, NODE_TYPE_WHILE_EXPR, while_token);
    result->data.while_expr.condition = condition;
    result->data.while_expr.continue_expression = continue_expression;
    if (opt_payload.unwrap(&payload))
    {
        result->data.while_expr.var_symbol = token_buffer(payload.payload);
        result->data.while_expr.var_is_ptr = payload.asterisk != nullptr;
    }

    return result;
}

static ASTNode* AST_parse_while_statement(ParseContext* pc)
{
    ASTNode* result = AST_parse_while_prefix(pc);
    if (!result)
    {
        return nullptr;
    }

    ASTNode* body = AST_parse_block_expression(pc);
    bool requires_semi = false;
    if (!body)
    {
        requires_semi = true;
        body = AST_parse_assign_expression(pc);
    }

    if (!body)
    {
        Token* token = eat_token(pc);
        AST_error(pc, token, "Expected loop body, found '%s'", token_name(token->id));
    }

    Token* error_payload = nullptr;
    ASTNode* else_body = nullptr;
    if (eat_token_if(pc, TOKEN_ID_KEYWORD_ELSE))
    {
        error_payload = AST_parse_payload(pc);
        else_body = AST_expect(pc, AST_parse_statement);
    }

    if (requires_semi && else_body == nullptr)
    {
        expect_token(pc, TOKEN_ID_SEMICOLON);
    }

    assert(result->type = NODE_TYPE_WHILE_EXPR);
    result->data.while_expr.body = body;
    result->data.while_expr.error_symbol = token_buffer(error_payload);
    result->data.while_expr.else_node = else_body;

    return result;
}

static ASTNode* AST_parse_loop_statement(ParseContext* pc)
{
    Token* inline_token = eat_token_if(pc, TOKEN_ID_KEYWORD_INLINE);
    ASTNode* for_statement = AST_parse_for_statement(pc);
    if (for_statement)
    {
        assert(for_statement->type == NODE_TYPE_FOR_EXPR);
        for_statement->data.for_expr.is_inline = inline_token != nullptr;
        return for_statement;
    }

    ASTNode* while_statement = AST_parse_while_statement(pc);
    if (while_statement)
    {
        assert(while_statement->type = NODE_TYPE_WHILE_EXPR);
        while_statement->data.while_expr.is_inline = inline_token != nullptr;
        return while_statement;
    }

    if (inline_token)
    {
        AST_invalid_token_error(pc, peek_token(pc));
    }

    return nullptr;
}
static ASTNode* AST_parse_labeled_statement(ParseContext*pc)
{
    Token* label = AST_parse_block_label(pc);
    ASTNode* block = AST_parse_block(pc);
    if (block)
    {
        assert(block->type);
        block->data.block.name = token_buffer(label);
        return nullptr;
    }

    ASTNode* loop = AST_parse_loop_statement(pc);
    return nullptr;
}

static ASTNode* AST_parse_assign_op(ParseContext* pc)
{
    BinaryOpType table[TOKEN_ID_COUNT] = {};

    table[TOKEN_ID_BIT_AND_EQ] = BIN_OP_TYPE_ASSIGN_BIT_AND;
    table[TOKEN_ID_BIT_OR_EQ] = BIN_OP_TYPE_ASSIGN_BIT_OR;
    table[TOKEN_ID_BIT_SHL_EQ] = BIN_OP_TYPE_ASSIGN_BIT_SHIFT_LEFT;
    table[TOKEN_ID_BIT_SHR_EQ] = BIN_OP_TYPE_ASSIGN_BIT_SHIFT_RIGHT;
    table[TOKEN_ID_BIT_XOR_EQ] = BIN_OP_TYPE_ASSIGN_BIT_XOR;
    table[TOKEN_ID_DIV_EQ] = BIN_OP_TYPE_ASSIGN_DIV;
    table[TOKEN_ID_EQ] = BIN_OP_TYPE_ASSIGN;
    table[TOKEN_ID_MINUS_EQ] = BIN_OP_TYPE_ASSIGN_MINUS;
    table[TOKEN_ID_MOD_EQ] = BIN_OP_TYPE_ASSIGN_MOD;
    table[TOKEN_ID_PLUS_EQ] = BIN_OP_TYPE_ASSIGN_PLUS;
    table[TOKEN_ID_TIMES_EQ] = BIN_OP_TYPE_ASSIGN_TIMES;

    BinaryOpType op = table[peek_token(pc)->id];
    if (op != BIN_OP_TYPE_INVALID)
    {
        Token *op_token = eat_token(pc);
        ASTNode *res = AST_create_node(pc, NODE_TYPE_BIN_OP_EXPR, op_token);
        res->data.bin_op_expr.op = op;
        return res;
    }

    return nullptr;
}

static ASTNode* AST_parse_assign_expression(ParseContext* pc)
{
    return AST_parse_binary_op_expression(pc, BINARY_OP_CHAIN_ONCE, AST_parse_assign_op, AST_parse_expression);
}

static ASTNode* AST_parse_if_statement(ParseContext* pc)
{
    ASTNode* result = AST_parse_if_statement(pc);
    if (!result)
    {
        return nullptr;
    }

    ASTNode* body = AST_parse_block_expression(pc);
    bool requires_semi = false;
    if (!body)
    {
        requires_semi = true;
        body = AST_parse_assign_expression(pc);
    }

    if (!body)
    {
        Token* token = eat_token(pc);
        AST_error(pc, token, "Expected if body, found '%s'", token_name(token->id));
    }

    Token* error_payload = nullptr;
    ASTNode* else_body = nullptr;
    if (eat_token_if(pc, TOKEN_ID_KEYWORD_ELSE))
    {
        error_payload = AST_parse_payload(pc);
        else_body = AST_expect(pc, AST_parse_statement);
    }

    if (requires_semi && !else_body)
    {
        expect_token(pc, TOKEN_ID_SEMICOLON);
    }

    // TODO: fix this mess
//    assert(result->type == NODE_TYPE_IF_OPTIONAL);
//    if (error_payload)
//    {
//        ASTNodeTestExpression old = result->data.test_expr;
//        result->type = NODE_TYPE_IF_ERROR_EXPR;
//        result->data.if_
//    }
    if (result->data.test_expr.var_symbol)
    {
        result->data.test_expr.true_node = body;
        result->data.test_expr.false_node = else_body;
        return result;
    }

    ASTNodeTestExpression old = result->data.test_expr;
    result->type = NODE_TYPE_IF_BOOL_EXPR;
    result->data.if_bool_expr.condition = old.target_node;
    result->data.if_bool_expr.true_block = body;
    result->data.if_bool_expr.false_block = else_body;

    return result;
}

static Token* AST_parse_payload(ParseContext*pc)
{
    if (eat_token_if(pc, TOKEN_ID_BIT_OR) == nullptr)
    {
        return nullptr;
    }

    Token* result = expect_token(pc, TOKEN_ID_SYMBOL);
    expect_token(pc, TOKEN_ID_BIT_OR);

    return result;
}
static ASTNode* AST_parse_expression(ParseContext* pc)
{
    assert(0);
    return nullptr;
}

ASTNode* AST_parse_function_call_arguments(ParseContext* pc)
{
    Token* parenthesis = eat_token_if(pc, TOKEN_ID_LEFT_PARENTHESIS);
    if (!parenthesis)
    {
        return nullptr;
    }

    List<ASTNode*> params = AST_parse_list(pc, TOKEN_ID_COMMA, AST_parse_expression);
    expect_token(pc, TOKEN_ID_RIGHT_PARENTHESIS);

    ASTNode* result = AST_create_node(pc, NODE_TYPE_FN_CALL_EXPR, parenthesis);
    result->data.fn_call_expr.parameters = params;
    result->data.fn_call_expr.seen = false;

    return result;
}

ASTNode*AST_parse_suffix_op(ParseContext* pc)
{
    Token* left_bracket = eat_token_if(pc, TOKEN_ID_LEFT_BRACKET);
    if (left_bracket)
    {
        ASTNode* start = AST_expect(pc, AST_parse_expression);
        ASTNode* end = nullptr;

        // TODO: ellipsis2 code

        expect_token(pc, TOKEN_ID_RIGHT_BRACKET);

        ASTNode* result = AST_create_node(pc, NODE_TYPE_ARRAY_ACCESS_EXPR, left_bracket);
        result->data.array_access_expr.subscript = start;
        return result;
    }

    Token* dot = eat_token_if(pc, TOKEN_ID_DOT);
    if (dot)
    {
        Token* ident = expect_token(pc, TOKEN_ID_SYMBOL);
        ASTNode* result = AST_create_node(pc, NODE_TYPE_FIELD_ACCESS_EXPR, dot);
        result->data.field_access_expr.field_name = token_buffer(ident);

        return result;
    }

    return nullptr;
}
static ASTNode* AST_parse_container_declaration_type(ParseContext* pc)
{
    Token* first = eat_token_if(pc, TOKEN_ID_KEYWORD_STRUCT);
    if (first)
    {
        ASTNode* result = AST_create_node(pc, NODE_TYPE_CONTAINER_DECL, first);
        result->data.container_decl.init_argument_expression = nullptr;
        result->data.container_decl.type = CONTAINER_TYPE_STRUCT;
        return result;
    }

    first = eat_token_if(pc, TOKEN_ID_KEYWORD_ENUM);
    if (first)
    {
        ASTNode* init_argument_expression = nullptr;
        if (eat_token_if(pc, TOKEN_ID_LEFT_PARENTHESIS))
        {
            init_argument_expression = AST_expect(pc, AST_parse_expression);
            expect_token(pc, TOKEN_ID_RIGHT_PARENTHESIS);
        }
        ASTNode* result = AST_create_node(pc, NODE_TYPE_CONTAINER_DECL, first);
        result->data.container_decl.init_argument_expression = init_argument_expression;
        result->data.container_decl.type = CONTAINER_TYPE_ENUM;

        return result;
    }

    first = eat_token_if(pc, TOKEN_ID_KEYWORD_UNION);
    if (first)
    {
        ASTNode* result = AST_create_node(pc, NODE_TYPE_CONTAINER_DECL, first);
        result->data.container_decl.init_argument_expression = nullptr;
        result->data.container_decl.type = CONTAINER_TYPE_UNION;
        return result;
    }

    return nullptr;
}
static ASTNode* AST_parse_container_declaration_auto(ParseContext* pc)
{
    ASTNode* result = AST_parse_container_declaration_type(pc);
    if (!result)
    {
        return nullptr;
    }

    expect_token(pc, TOKEN_ID_LEFT_BRACE);
    ASTNodeContainerDeclaration members = AST_parse_container_members(pc);
    expect_token(pc, TOKEN_ID_RIGHT_BRACE);

    result->data.container_decl.fields = members.fields;
    result->data.container_decl.declarations = members.declarations;
    if (buf_len(&members.document_comments) != 0)
    {
        result->data.container_decl.document_comments = members.document_comments;
    }

    return result;
}

static ASTNode* AST_parse_container_declaration(ParseContext* pc)
{
    Token* layout_token = eat_token_if(pc, TOKEN_ID_KEYWORD_EXTERN);
    if (!layout_token)
    {
        layout_token = eat_token_if(pc, TOKEN_ID_KEYWORD_PACKED);
    }

    ASTNode* result = AST_parse_container_declaration_auto(pc);
    if (!result)
    {
        if (layout_token)
        {
            put_back_token(pc);
        }
        return nullptr;
    }

    assert(result->type = NODE_TYPE_CONTAINER_DECL);
    if (layout_token)
    {
        result->line = layout_token->start_line;
        result->column = layout_token->start_column;
        result->data.container_decl.layout = layout_token->id == TOKEN_ID_KEYWORD_EXTERN
                ? CONTAINER_LAYOUT_EXTERN
                : CONTAINER_LAYOUT_PACKED;
    }

    return result;

}

static ASTNode* AST_parse_field_init(ParseContext* pc)
{
    Token* first = eat_token_if(pc, TOKEN_ID_DOT);
    if (!first)
    {
        return nullptr;
    }

    Token* name = eat_token_if(pc, TOKEN_ID_SYMBOL);
    if (!name)
    {
        put_back_token(pc);
        return nullptr;
    }

    if (!eat_token_if(pc, TOKEN_ID_EQ))
    {
        put_back_token(pc);
        put_back_token(pc);
        return nullptr;
    }

    ASTNode* expression = AST_expect(pc, AST_parse_expression);
    ASTNode* result = AST_create_node(pc, NODE_TYPE_STRUCT_VALUE_FIELD, first);
    result->data.struct_val_field.name = token_buffer(name);
    result->data.struct_val_field.expression = expression;

    return result;
}

static ASTNode* AST_parse_init_list(ParseContext* pc)
{
    Token* left_brace = eat_token_if(pc, TOKEN_ID_LEFT_BRACE);
    if (!left_brace)
    {
        return nullptr;
    }

    ASTNode*first = AST_parse_field_init(pc);
    if (first)
    {
        ASTNode* result = AST_create_node(pc, NODE_TYPE_CONTAINER_INIT_EXPR, left_brace);
        result->data.container_init_expr.init_type = CONTAINER_INITIALIZATION_TYPE_STRUCT;
        result->data.container_init_expr.entries.append(first);

        while (eat_token_if(pc, TOKEN_ID_COMMA))
        {
            ASTNode* field_init = AST_parse_field_init(pc);
            if (!field_init)
            {
                break;
            }
            result->data.container_init_expr.entries.append(field_init);
        }

        expect_token(pc, TOKEN_ID_RIGHT_BRACE);
        return result;
    }

    ASTNode* result = AST_create_node(pc, NODE_TYPE_CONTAINER_INIT_EXPR, left_brace);
    result->data.container_init_expr.init_type = CONTAINER_INITIALIZATION_TYPE_ARRAY;

    first = AST_parse_expression(pc);
    if (first)
    {
        result->data.container_init_expr.entries.append(first);

        while (eat_token_if(pc, TOKEN_ID_COMMA))
        {
            ASTNode* expression = AST_parse_expression(pc);
            if (!expression)
            {
                break;
            }
            result->data.container_init_expr.entries.append(expression);
        }
    }

    expect_token(pc, TOKEN_ID_RIGHT_BRACE);
    return result;
}

static ASTNode* AST_parse_anonymous_literal(ParseContext* pc)
{
    Token* period = eat_token_if(pc, TOKEN_ID_DOT);
    if (!period)
    {
        return nullptr;
    }

    Token* identifier = eat_token_if(pc, TOKEN_ID_SYMBOL);
    if (identifier)
    {
        ASTNode* result = AST_create_node(pc, NODE_TYPE_ENUM_LITERAL, period);
        result->data.enum_literal.period = period;
        result->data.enum_literal.identifier = identifier;
        return result;
    }

    ASTNode* result = AST_parse_init_list(pc);
    if (result)
    {
        return result;
    }

    put_back_token(pc);
    return nullptr;
}

static Token* AST_parse_doc_comments(ParseContext* pc, Buffer* buffer)
{
    Token* first_doc_token = nullptr;
    Token* doc_token = nullptr;

    assert(0);
    //while ((doc_token = eat_token_if(pc, do)))
    return nullptr;
}
static ASTNode* AST_parse_error_set_declaration(ParseContext* pc)
{
    Token* first = eat_token_if(pc, TOKEN_ID_KEYWORD_ERROR);
    if (!first)
    {
        return nullptr;
    }
    if (!eat_token_if(pc, TOKEN_ID_LEFT_BRACE))
    {
        put_back_token(pc);
        return nullptr;
    }

    List<ASTNode*> declarations = AST_parse_list<ASTNode>(pc, TOKEN_ID_COMMA, [](ParseContext* parse_context)
    {
        Buffer doc_comment_buffer = {{0}};
        Token* doc_token = AST_parse_doc_comments(parse_context, &doc_comment_buffer);
        Token* ident = eat_token_if(parse_context, TOKEN_ID_SYMBOL);
        if (!ident)
        {
            return (ASTNode*)nullptr;
        }

        ASTNode* symbol_node = token_symbol(parse_context, ident);
        if (!doc_token)
        {
            return symbol_node;
        }

        ASTNode* field_node = AST_create_node(parse_context, NODE_TYPE_ERROR_SET_FIELD, doc_token);
        field_node->data.err_set_field.field_name = symbol_node;
        field_node->data.err_set_field.document_comments = doc_comment_buffer;

        return field_node;
    });

    expect_token(pc, TOKEN_ID_RIGHT_BRACE);

    ASTNode* result = AST_create_node(pc, NODE_TYPE_ERROR_SET_DECL, first);
    result->data.err_set_decl.declarations = declarations;

    return result;
}

static ASTNode* AST_parse_grouped_expression(ParseContext* pc)
{
    Token* left_parenthesis = eat_token_if(pc, TOKEN_ID_LEFT_PARENTHESIS);
    if (!left_parenthesis)
    {
        return nullptr;
    }

    ASTNode* expression = AST_expect(pc, AST_parse_expression);
    expect_token(pc, TOKEN_ID_RIGHT_PARENTHESIS);

    ASTNode* result = AST_create_node(pc, NODE_TYPE_GROUPED_EXPR, left_parenthesis);
    result->data.grouped_expr = expression;

    return result;
}

static ASTNode* AST_parse_for_expression_helper(ParseContext* pc, ASTNode* (*body_parser)(ParseContext*))
{
    ASTNode* result = AST_parse_for_prefix(pc);
    if (!result)
    {
        return nullptr;
    }

    ASTNode* body = AST_expect(pc, body_parser);
    ASTNode* else_body = nullptr;
    if (eat_token_if(pc, TOKEN_ID_KEYWORD_ELSE))
    {
        else_body = AST_expect(pc, body_parser);
    }

    assert(result->type == NODE_TYPE_FOR_EXPR);
    result->data.for_expr.body = body;
    result->data.for_expr.else_node = else_body;

    return result;
}

static ASTNode* AST_parse_while_expression_helper(ParseContext* pc, ASTNode* (*body_parser)(ParseContext*))
{
    ASTNode* result = AST_parse_while_prefix(pc);
    if (!result)
    {
        return nullptr;
    }

    ASTNode* body = AST_expect(pc, body_parser);
    Token* error_payload = nullptr;
    ASTNode* else_body = nullptr;

    if (eat_token_if(pc, TOKEN_ID_KEYWORD_ELSE))
    {
        error_payload = AST_parse_payload(pc);
        else_body = AST_expect(pc, body_parser);
    }

    assert(result->type == NODE_TYPE_WHILE_EXPR);
    result->data.while_expr.body;
    result->data.while_expr.error_symbol = token_buffer(error_payload);
    result->data.while_expr.else_node = else_body;

    return result;
}

static ASTNode* AST_parse_for_type_expression(ParseContext* pc)
{
    return AST_parse_for_expression_helper(pc, AST_parse_type_expression);
}

static ASTNode* AST_parse_while_type_expression(ParseContext* pc)
{
    return AST_parse_while_expression_helper(pc, AST_parse_type_expression);
}

static ASTNode* AST_parse_loop_expression_helper(ParseContext* pc,
    ASTNode* (*for_parser)(ParseContext*),
    ASTNode* (*while_parser)(ParseContext*))
{
    Token* inline_token = eat_token_if(pc, TOKEN_ID_KEYWORD_INLINE);
    ASTNode* for_expression = for_parser(pc);
    if (for_expression)
    {
        assert(for_expression->type == NODE_TYPE_FOR_EXPR);
        for_expression->data.for_expr.is_inline = inline_token != nullptr;
        return for_expression;
    }

    ASTNode* while_expression = while_parser(pc);
    if (while_expression)
    {
        assert(while_expression->type == NODE_TYPE_WHILE_EXPR);
        while_expression->data.while_expr.is_inline = inline_token != nullptr;
        return while_expression;
    }

    if (inline_token)
    {
        AST_invalid_token_error(pc, peek_token(pc));
    }
    return nullptr;
}

static ASTNode* AST_parse_loop_type_expression(ParseContext* pc)
{
    return AST_parse_loop_expression_helper(pc, AST_parse_for_type_expression, AST_parse_while_type_expression);
}

static ASTNode* AST_parse_labeled_type_expression(ParseContext* pc)
{
    Token* label = AST_parse_block_label(pc);
    if (label)
    {
        ASTNode* block = AST_parse_block(pc);
        if (block)
        {
            assert(block->type == NODE_TYPE_BLOCK);
            block->data.block.name = token_buffer(label);
            return block;
        }
    }

    ASTNode* loop = AST_parse_loop_type_expression(pc);
    if (loop)
    {
        switch (loop->type)
        {
            case NODE_TYPE_FOR_EXPR:
                loop->data.for_expr.name = token_buffer(label);
                break;
            case NODE_TYPE_WHILE_EXPR:
                loop->data.while_expr.name = token_buffer(label);
                break;
            default:
                RED_UNREACHABLE;
        }
        return loop;
    }

    if (label)
    {
        put_back_token(pc);
        put_back_token(pc);
    }

    return nullptr;
}

static ASTNode* AST_parse_if_prefix(ParseContext* pc)
{
    Token* first = eat_token_if(pc, TOKEN_ID_KEYWORD_IF);
    if (!first)
    {
        return nullptr;
    }

    expect_token(pc, TOKEN_ID_LEFT_PARENTHESIS);
    ASTNode* condition = AST_expect(pc, AST_parse_expression);
    expect_token(pc, TOKEN_ID_RIGHT_PARENTHESIS);
    Optional<PointerPayload> opt_payload = AST_parse_ptr_payload(pc);

    PointerPayload payload;
    ASTNode* result = AST_create_node(pc, NODE_TYPE_IF_OPTIONAL, first);
    result->data.test_expr.target_node = condition;
    if (opt_payload.unwrap(&payload))
    {
        result->data.test_expr.var_symbol = token_buffer(payload.payload);
        result->data.test_expr.var_is_ptr = payload.asterisk != nullptr;
    }

    return result;
}

static ASTNode* AST_parse_if_expression_helper(ParseContext* pc, ASTNode* (*body_parser)(ParseContext*))
{
    ASTNode* result = AST_parse_if_prefix(pc);
    if (!result)
    {
        return nullptr;
    }

    ASTNode* body = AST_expect(pc, body_parser);
    Token* error_payload = nullptr;
    ASTNode* else_body = nullptr;
    if (eat_token_if(pc, TOKEN_ID_KEYWORD_ELSE))
    {
        error_payload = AST_parse_payload(pc);
        else_body = AST_expect(pc, body_parser);
    }

    assert(result->type == NODE_TYPE_IF_OPTIONAL);
    if (error_payload)
    {
        //ASTNodeErrorType old = result->data.test_expr;
        //result->type = NODE_TYPE_IF_ERROR_EXPR;
    }
    RED_NOT_IMPLEMENTED;
    return nullptr;
}
static ASTNode* AST_parse_if_type_expression(ParseContext* pc)
{
    return AST_parse_if_expression_helper(pc, AST_parse_type_expression);
}

static ASTNode* AST_parse_primary_type_expression(ParseContext* pc)
{
    // TODO: This is not in line with the grammar.
    //       Because the prev stage 1 tokenizer does not parse
    //       @[a-zA-Z_][a-zA-Z0-9_] as one token, it has to do a
    //       hack, where it accepts '@' (IDENTIFIER / KEYWORD_export).
    //       I'd say that it's better if '@' is part of the builtin
    //       identifier token.
    Token *at_sign = eat_token_if(pc, TOKEN_ID_AT);
    if (at_sign != nullptr) {
        Buffer *name;
        Token *token = eat_token_if(pc, TOKEN_ID_KEYWORD_EXPORT);
        if (token == nullptr) {
            token = expect_token(pc, TOKEN_ID_SYMBOL);
            name = token_buffer(token);
        } else {
            name = buf_create_from_str("export");
        }

        ASTNode *res = AST_expect(pc, AST_parse_function_call_arguments);
        ASTNode *name_sym = AST_create_node(pc, NODE_TYPE_SYMBOL, token);
        name_sym->data.symbol_expr.symbol = name;

        assert(res->type == NODE_TYPE_FN_CALL_EXPR);
        res->line = at_sign->start_line;
        res->column = at_sign->start_column;
        res->data.fn_call_expr.function = name_sym;
        res->data.fn_call_expr.modifier = CALL_MODIFIER_BUILTIN;
        return res;
    }

    Token *char_lit = eat_token_if(pc, TOKEN_ID_CHAR_LIT);
    if (char_lit != nullptr) {
        ASTNode *res = AST_create_node(pc, NODE_TYPE_CHAR_LITERAL, char_lit);
        res->data.char_literal.value = char_lit->data.char_lit.c;
        return res;
    }

    ASTNode *container_decl = AST_parse_container_declaration(pc);
    if (container_decl != nullptr)
        return container_decl;

    ASTNode *anon_lit = AST_parse_anonymous_literal(pc);
    if (anon_lit != nullptr)
        return anon_lit;

    ASTNode *error_set_decl = AST_parse_error_set_declaration(pc);
    if (error_set_decl != nullptr)
        return error_set_decl;

    Token *float_lit = eat_token_if(pc, TOKEN_ID_FLOAT_LIT);
    if (float_lit != nullptr) {
        ASTNode *res = AST_create_node(pc, NODE_TYPE_FLOAT_LITERAL, float_lit);
        res->data.float_literal.big_float = &float_lit->data.float_lit.big_float;
        res->data.float_literal.overflow = float_lit->data.float_lit.overflow;
        return res;
    }

    ASTNode *fn_proto = AST_parse_function_prototype(pc);
    if (fn_proto != nullptr)
        return fn_proto;

    ASTNode *grouped_expr = AST_parse_grouped_expression(pc);
    if (grouped_expr != nullptr)
        return grouped_expr;

    ASTNode *labeled_type_expr = AST_parse_labeled_type_expression(pc);
    if (labeled_type_expr != nullptr)
        return labeled_type_expr;

    Token *identifier = eat_token_if(pc, TOKEN_ID_SYMBOL);
    if (identifier != nullptr)
        return token_symbol(pc, identifier);

    ASTNode *if_type_expr = AST_parse_if_type_expression(pc);
    if (if_type_expr != nullptr)
        return if_type_expr;

    Token *int_lit = eat_token_if(pc, TOKEN_ID_INT_LIT);
    if (int_lit != nullptr) {
        ASTNode *res = AST_create_node(pc, NODE_TYPE_INT_LITERAL, int_lit);
        res->data.int_literal.big_int = &int_lit->data.int_lit.big_int;
        return res;
    }

    Token *comptime = eat_token_if(pc, TOKEN_ID_KEYWORD_COMPTIME);
    if (comptime != nullptr) {
        ASTNode *expr = AST_expect(pc, AST_parse_type_expression);
        ASTNode *res = AST_create_node(pc, NODE_TYPE_COMP_TIME, comptime);
        res->data.comptime_expr.expression = expr;
        return res;
    }

    Token *error = eat_token_if(pc, TOKEN_ID_KEYWORD_ERROR);
    if (error != nullptr) {
        Token *dot = expect_token(pc, TOKEN_ID_DOT);
        Token *name = expect_token(pc, TOKEN_ID_SYMBOL);
        ASTNode *left = AST_create_node(pc, NODE_TYPE_ERROR_TYPE, error);
        ASTNode *res = AST_create_node(pc, NODE_TYPE_FIELD_ACCESS_EXPR, dot);
        res->data.field_access_expr.struct_ref = left;
        res->data.field_access_expr.field_name = token_buffer(name);
        return res;
    }

    Token *false_token = eat_token_if(pc, TOKEN_ID_KEYWORD_FALSE);
    if (false_token != nullptr) {
        ASTNode *res = AST_create_node(pc, NODE_TYPE_BOOL_LITERAL, false_token);
        res->data.bool_literal.value = false;
        return res;
    }

    Token *null = eat_token_if(pc, TOKEN_ID_KEYWORD_NULL);
    if (null != nullptr)
        return AST_create_node(pc, NODE_TYPE_NULL_LITERAL, null);

    Token *anyframe = eat_token_if(pc, TOKEN_ID_KEYWORD_ANY_FRAME);
    if (anyframe != nullptr)
        return AST_create_node(pc, NODE_TYPE_ANY_FRAME_TYPE, anyframe);

    Token *true_token = eat_token_if(pc, TOKEN_ID_KEYWORD_TRUE);
    if (true_token != nullptr) {
        ASTNode *res = AST_create_node(pc, NODE_TYPE_BOOL_LITERAL, true_token);
        res->data.bool_literal.value = true;
        return res;
    }

    Token *undefined = eat_token_if(pc, TOKEN_ID_KEYWORD_UNDEFINED);
    if (undefined != nullptr)
        return AST_create_node(pc, NODE_TYPE_UNDEFINED_LITERAL, undefined);

//    Token *unreachable = eat_token_if(pc, TokenIdKeywordUnreachable);
//    if (unreachable != nullptr)
//        return ast_create_node(pc, NODE_TYPE_UNREACHABLE, unreachable);

    Token *string_lit = eat_token_if(pc, TOKEN_ID_STRING_LIT);
    if (string_lit == nullptr)
        string_lit = eat_token_if(pc, TOKEN_ID_MULTILINE_STRING_LIT);
    if (string_lit != nullptr) {
        ASTNode *res = AST_create_node(pc, NODE_TYPE_STRING_LITERAL, string_lit);
        res->data.string_literal.buffer = token_buffer(string_lit);
        return res;
    }

    ASTNode *switch_expr = AST_parse_switch_expression(pc);
    if (switch_expr != nullptr)
        return switch_expr;

    return nullptr;
}

ASTNode*AST_parse_block_expression_statement(ParseContext*pc)
{
    ASTNode* block = AST_parse_block_expression(pc);
    if (block)
    {
        return block;
    }

    ASTNode* assign_expression = AST_parse_assign_expression(pc);
    if (assign_expression)
    {
        expect_token(pc, TOKEN_ID_SEMICOLON);
        return assign_expression;
    }

    return nullptr;
}

static ASTNode* AST_parse_parameter_type(ParseContext* pc)
{
    Token* any_type_token = eat_token_if(pc, TOKEN_ID_KEYWORD_ANY);
    if (any_type_token)
    {
        ASTNode* result = AST_create_node(pc, NODE_TYPE_PARAM_DECL, any_type_token);
        result->data.param_decl.any_type_token = any_type_token;
        return result;
    }

    // three dots for varargs

    ASTNode* type_expression = AST_parse_type_expression(pc);
    if (type_expression)
    {
        ASTNode* result = AST_create_node_copy_line_info(pc, NODE_TYPE_PARAM_DECL, type_expression);
        result->data.param_decl.type = type_expression;
        return result;
    }

    return nullptr;
}

static ASTNode* AST_parse_call_convention_expression(ParseContext*pc)
{
    Token* first = eat_token_if(pc, TOKEN_ID_KEYWORD_CALL_CONV);
    if (!first)
    {
        return nullptr;
    }

    expect_token(pc, TOKEN_ID_LEFT_PARENTHESIS);
    ASTNode* result = AST_expect(pc, AST_parse_expression);
    expect_token(pc, TOKEN_ID_RIGHT_PARENTHESIS);

    return result;
}

static ASTNode* AST_parse_parameter_declaration(ParseContext* pc)
{
    Buffer doc_comments = {{0}};
    AST_parse_doc_comments(pc, &doc_comments);

    Token* first = eat_token_if(pc, TOKEN_ID_KEYWORD_NO_ALIAS);
    if (!first)
    {
        first = eat_token_if(pc, TOKEN_ID_KEYWORD_COMPTIME);
    }

    Token* name = eat_token_if(pc, TOKEN_ID_SYMBOL);
    if (name)
    {
        if (eat_token_if(pc, TOKEN_ID_COLON))
        {
            if (!first)
            {
                first = name;
            }
        }
        else
        {
            put_back_token(pc);
            name = nullptr;
        }
    }

    ASTNode* result;
    if (!first)
    {
        first = peek_token(pc);
        result = AST_parse_parameter_type(pc);
    }
    else
    {
        result = AST_expect(pc, AST_parse_parameter_type);
    }

    if (!result)
    {
        return nullptr;
    }

    assert(result->type == NODE_TYPE_PARAM_DECL);
    result->line = first->start_line;
    result->column = first->start_column;
    result->data.param_decl.name = token_buffer(name);
    result->data.param_decl.document_comments = doc_comments;
    result->data.param_decl.is_noalias = first->id == TOKEN_ID_KEYWORD_NO_ALIAS;
    result->data.param_decl.is_compile_time = first->id == TOKEN_ID_KEYWORD_COMPTIME;

    return result;
}

ASTNode* AST_parse_type_expression(ParseContext* pc)
{
    return AST_parse_prefix_op_expression(pc, AST_parse_prefix_type_op, AST_parse_error_union_expression);
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

    ASTNode* result = AST_create_node(pc, NODE_TYPE_FN_PROTO, first);
    result->data.fn_prototype = {};
    result->data.fn_prototype.name = token_buffer(identifier);
    result->data.fn_prototype.parameters = params;
    result->data.fn_prototype.align_expression = align_expression;
    result->data.fn_prototype.section_expression = section_expression;
    result->data.fn_prototype.call_convention_expression = call_convention_expression;
    result->data.fn_prototype.return_any_type_token = any_type;
    result->data.fn_prototype.auto_err_set = exmark != nullptr;
    result->data.fn_prototype.return_type = return_type;

    for (size_t i = 0; i < params.length; i++)
    {
        ASTNode* param_declaration = params.at(i);
        assert(param_declaration->type == NODE_TYPE_PARAM_DECL);
        if (param_declaration->data.param_decl.is_var_args)
        {
            result->data.fn_prototype.is_var_args = true;
        }
        if (i != params.length - 1 && result->data.fn_prototype.is_var_args)
        {
            AST_error(pc, first, "Function prototype has varargs as non-last parameter");
        }
    }

    return result;
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
