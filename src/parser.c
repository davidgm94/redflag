/*
    * Fn declaration - NO
    * Fn prototype - OK
    * Type = OK
    * Parameter declaration = OK
    * Block = OK
    * Return expression = OK
    * Variable declaration = OK
    * Symbol expression = NO
    * Binary operation expression = OK
    * Function call expression = NO
    * Branch expression = OK
    * Int literal = OK
*/
#include "compiler_types.h"
#include "parser.h"
#include "lexer.h"
#include "os.h"
#include <stdarg.h>
#include <stdio.h>

typedef ASTNode Node;


static inline void copy_base_node(ASTNode* dst, const ASTNode* src, AST_ID id)
{
    dst->node_id = id;
    dst->node_line = src->node_line;
    dst->node_column = src->node_column;
    static_assert(offsetof(ASTNode, sym_decl) == (sizeof(dst->node_id) + sizeof(dst->node_line) + sizeof(dst->node_column) + sizeof(dst->node_padding)), "Base node size has been incorrectly modified");
}

static inline void fill_base_node(ASTNode* bn, Token* t, AST_ID id)
{
    bn->node_id = id;
    bn->node_line = t->start_line;
    bn->node_column = t->start_column;
}

static inline AST_ID get_node_type(ASTNode* n)
{
    return n->node_id;
}


static inline ASTNode* create_symbol_node(Token* t)
{
    ASTNode* node = NEW(ASTNode, 1);
    fill_base_node(node, t, AST_TYPE_SYM_EXPR);
    node->sym_expr.name = &t->str_lit.str;
    return node;
}

typedef struct ParseContext
{
    SB* src_buffer;
    TokenBuffer* token_buffer;
    size_t current_token;
    //RedType* owner;
} ParseContext;

static void error(ParseContext* pc, Token* token, const char* format, ...)
{
    fprintf(stdout, "error parsing token %s at line %zu column %zu: ", token_name(token->id), token->start_line + 1, token->start_column + 1);
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
    os_exit(1);
}

static inline void invalid_token_error(ParseContext* pc, Token* token)
{
    error(pc, token, "invalid token: '%s'", token_name(token->id));
}

static inline Token* get_token_i(ParseContext* pc, size_t i)
{
    return &pc->token_buffer->ptr[pc->current_token + i];
}

static inline Token* get_token(ParseContext* pc)
{
    return get_token_i(pc, 0);
}

static inline Token* consume_token(ParseContext* pc)
{
    Token* token = get_token(pc);

#if RED_PARSER_VERBOSE
    PRINT_TOKEN_WITH_PREFIX("Consuming", token, pc->current_token, symbol_name);
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
    return NULL;
}

static inline Token* expect_token(ParseContext* pc, TokenID id)
{
    Token* token = consume_token(pc);
    if (token->id != id)
    {
        error(pc, token, "expected token '%s', found '%s'", token_name(id), token_name(token->id));
    }

    return token;
}

static inline Token* expect_token_if_not(ParseContext* pc, TokenID expected_token, TokenID if_not_this_one)
{
    Token* token = get_token(pc);
    if (token->id != if_not_this_one)
    {
        token = expect_token(pc, expected_token);
        return token;
    }

    return null;
}

static inline void put_back_token(ParseContext* pc)
{
#if RED_PARSER_VERBOSE
    Token* wrong_token = get_token(pc);
#endif
    pc->current_token -= 1;
#if RED_PARSER_VERBOSE
    Token* good_token = get_token(pc);
    StringBuffer* wrong_symbol = wrong_token->id == TOKEN_ID_SYMBOL ? token_buffer(wrong_token) : NULL;
    StringBuffer* good_symbol = good_token->id == TOKEN_ID_SYMBOL ? token_buffer(good_token) : NULL;
    print("Current token #%zu: %s name: %s ******** Putting back token #%zu: %s name: %s\n", pc->current_token + 1, token_name(wrong_token->id), wrong_symbol ? wrong_symbol->ptr : "not a symbol", pc->current_token, token_name(good_token->id), good_symbol ? good_symbol->ptr : "not a symbol");
#endif
}

static inline ASTNode* parse_expression(ParseContext* pc);
static inline ASTNode* parse_primary_expr(ParseContext* pc);
static inline ASTNode* parse_compound_st(ParseContext* pc);
static inline ASTNode* create_type_node(ParseContext* pc);

static inline ASTNode* create_basic_type_node(ParseContext* pc)
{
    Token* token = consume_token_if(pc, TOKEN_ID_SYMBOL);
    if (!token)
    {
        return null;
    }

    ASTNode* node = NEW(ASTNode, 1);
    fill_base_node(node, token, AST_TYPE_TYPE_EXPR);
    node->type_expr.kind = TYPE_KIND_PRIMITIVE;
    node->type_expr.name = &token->str_lit.str;
    return node;
}

static inline ASTNode* create_type_node_array(ParseContext* pc)
{
    Token* left_bracket = expect_token(pc, TOKEN_ID_LEFT_BRACKET);
    ASTNode* elem_count_node = parse_expression(pc);
    expect_token(pc, TOKEN_ID_RIGHT_BRACKET);
    ASTNode* type_node = create_type_node(pc);

    ASTNode* node = NEW(ASTNode, 1);
    fill_base_node(node, left_bracket, AST_TYPE_TYPE_EXPR);
    node->type_expr.kind = TYPE_KIND_ARRAY;
    node->type_expr.array.element_count_expr = elem_count_node;
    node->type_expr.array.type = type_node;

    return node;
}

const char* primitive_types[] =
{
    "u8", "u16", "u32", "u64",
    "s8", "s16", "s32", "s64",
    "f32", "f64",
};

static bool is_basic_type(Token* type_token)
{
    char* type_str = type_token->str_lit.str.ptr;
    for (u32 i = 0; i < array_length(primitive_types); i++)
    {
        if (strcmp(type_str, primitive_types[i]) == 0)
        {
            return true;
        }
    }
    return false;
}

static inline ASTNode* create_complex_type_node(ParseContext* pc)
{
    Token* token = consume_token_if(pc, TOKEN_ID_SYMBOL);
    ASTNode* node = NEW(ASTNode, 1);
    fill_base_node(node, token, AST_TYPE_TYPE_EXPR);
    node->type_expr.kind = TYPE_KIND_COMPLEX_TO_BE_DETERMINED;
    node->type_expr.name = &token->str_lit.str;
    return node;
}

static inline ASTNode* create_type_node_pointer(ParseContext* pc)
{
    Token* p_token = expect_token(pc, TOKEN_ID_AMPERSAND);
    ASTNode* node = NEW(Node, 1);
    fill_base_node(node, p_token, AST_TYPE_TYPE_EXPR);
    node->type_expr.kind = TYPE_KIND_POINTER;
    node->type_expr.pointer_.type = create_type_node(pc);

    return node;
}

static inline ASTNode* create_type_node(ParseContext* pc)
{
    Token* token = get_token(pc);
    TokenID type = token->id;
    switch (type)
    {
        case TOKEN_ID_SYMBOL:
            if (is_basic_type(token))
            {
                return create_basic_type_node(pc);
            }
            else
            {
                return create_complex_type_node(pc);
            }
        case TOKEN_ID_LEFT_BRACKET:
            return create_type_node_array(pc);
        case TOKEN_ID_AMPERSAND:
            return create_type_node_pointer(pc);
        default:
            RED_NOT_IMPLEMENTED;
            return null;
    }
}

static inline ASTNode* parse_param_decl(ParseContext* pc)
{
    Token* name = expect_token(pc, TOKEN_ID_SYMBOL);
    ASTNode* symbol_node = create_symbol_node(name);
    ASTNode* type_node = create_type_node(pc);
    if (get_token(pc)->id != TOKEN_ID_RIGHT_PARENTHESIS)
    {
        expect_token(pc, TOKEN_ID_COMMA);
    }

    ASTNode* param = NEW(ASTNode, 1);
    fill_base_node(param, name, AST_TYPE_PARAM_DECL);
    param->param_decl.sym = symbol_node;
    param->param_decl.type = type_node;
    return param;
}

static inline ASTNodeBuffer parse_param_decl_list(ParseContext* pc)
{
    // Left parenthesis is already consumed
    //expect_token(pc, TOKEN_ID_LEFT_PARENTHESIS);

    ASTNodeBuffer nb = ZERO_INIT;
    while (get_token(pc)->id != TOKEN_ID_RIGHT_PARENTHESIS)
    {
        ASTNode* param = parse_param_decl(pc);
        if (param)
        {
            node_append(&nb, param);
        }
        else
        {
            os_exit(1);
        }
    }

    expect_token(pc, TOKEN_ID_RIGHT_PARENTHESIS);
    return nb;
}

static inline ASTNode* parse_sym_decl(ParseContext* pc)
{
    Token* mut_token = consume_token_if(pc, TOKEN_ID_KEYWORD_CONST);
    if (!mut_token)
    {
        mut_token = consume_token_if(pc, TOKEN_ID_KEYWORD_VAR);
        if (!mut_token)
        {
            return null;
        }
    }

    bool is_const = mut_token->id == TOKEN_ID_KEYWORD_CONST;
    Token* sym_name = expect_token(pc, TOKEN_ID_SYMBOL);
    // TODO: should flexibilize this in order to support type inferring in the future
    ASTNode* sym_type_node = create_type_node(pc);

    // TODO: This means no value assigned, uninitialized (left to the backend?????)
    ASTNode* sym_node = null;
    Token* semicolon = consume_token_if(pc, TOKEN_ID_SEMICOLON);
    if (semicolon)
    {
        sym_node = NEW(ASTNode, 1);
        fill_base_node(sym_node, mut_token, AST_TYPE_SYM_DECL);
        sym_node->sym_decl.is_const = is_const;
        sym_node->sym_decl.sym = create_symbol_node(sym_name);
        sym_node->sym_decl.type = sym_type_node;
        return sym_node;
    }

    expect_token(pc, TOKEN_ID_EQ);
    ASTNode* expression = parse_expression(pc);
    expect_token(pc, TOKEN_ID_SEMICOLON);

    sym_node = NEW(ASTNode, 1);
    fill_base_node(sym_node, mut_token, AST_TYPE_SYM_DECL);
    sym_node->sym_decl.is_const = is_const;
    sym_node->sym_decl.sym = create_symbol_node(sym_name);
    sym_node->sym_decl.type = sym_type_node;
    sym_node->sym_decl.value = expression;

    return sym_node;
}

/*
Var declaration
Branch (if-else)
return expression
variable assignment (bin op?)
*/
static inline ASTNode* parse_int_literal(ParseContext* pc)
{
    Token* token = consume_token_if(pc, TOKEN_ID_INT_LIT);
    if (!token)
    {
        return null;
    }

    ASTNode* node = NEW(ASTNode, 1);
    fill_base_node(node, token, AST_TYPE_INT_LIT);
    node->int_lit.bigint = token_bigint(token);

    return node;
}

static inline ASTNode* parse_symbol_expr(ParseContext* pc)
{
    Token* token = consume_token_if(pc, TOKEN_ID_SYMBOL);
    if (!token)
    {
        return null;
    }

    ASTNode* node = create_symbol_node(token);

    if (consume_token_if(pc, TOKEN_ID_LEFT_BRACKET))
    {
        ASTNode* array_access = parse_expression(pc);
        expect_token(pc, TOKEN_ID_RIGHT_BRACKET);
        node->sym_expr.subscript_type = AST_SYMBOL_SUBSCRIPT_TYPE_ARRAY_ACCESS;
        node->sym_expr.subscript = array_access;
    }
    else if (consume_token_if(pc, TOKEN_ID_DOT))
    {
        ASTNode* field_access = parse_primary_expr(pc);
        node->sym_expr.subscript_type = AST_SYMBOL_SUBSCRIPT_TYPE_FIELD_ACCESS;
        node->sym_expr.subscript = field_access;
    }

    return node;
}

static inline ASTNode* parse_branch_block(ParseContext* pc)
{
    ASTNode* branch_block = NULL;
    Token* if_token = expect_token(pc, TOKEN_ID_KEYWORD_IF);

    ASTNode* condition_node = parse_expression(pc);
    if (!condition_node)
    {
        return null;
    }

    ASTNode* if_block = parse_expression(pc);
    if (!if_block)
    {
        RED_NOT_IMPLEMENTED;
        return null;
    }

    Token* else_token = consume_token_if(pc, TOKEN_ID_KEYWORD_ELSE);
    if (!else_token)
    {
        branch_block = NEW(ASTNode, 1);
        fill_base_node(branch_block, if_token, AST_TYPE_BRANCH_EXPR);
        branch_block->branch_expr.condition = condition_node;
        branch_block->branch_expr.if_block = if_block;
        branch_block->branch_expr.else_block = null;

        return branch_block;
    }

    Token* else_if_token = get_token(pc);
    if (else_if_token->id != TOKEN_ID_KEYWORD_IF)
    {
        // IF-ELSE BLOCK

        // parse else block
        ASTNode* else_block = parse_expression(pc);
        if (!else_block)
        {
            RED_NOT_IMPLEMENTED;
            return null;
        }
        redassert(else_block->node_id == AST_TYPE_COMPOUND_STATEMENT);

        branch_block = NEW(ASTNode, 1);
        fill_base_node(branch_block, if_token, AST_TYPE_BRANCH_EXPR);
        branch_block->branch_expr.condition = condition_node;
        branch_block->branch_expr.if_block = if_block;
        branch_block->branch_expr.else_block = else_block;

        return branch_block;
    }

    branch_block = NEW(ASTNode, 1);
    fill_base_node(branch_block, if_token, AST_TYPE_BRANCH_EXPR);
    branch_block->branch_expr.condition = condition_node;
    branch_block->branch_expr.if_block = if_block;
    ASTNode* branch_it = branch_block;
    do
    {
        ASTNode* new_branch_block;
        else_if_token = get_token(pc);
        if (else_if_token->id == TOKEN_ID_KEYWORD_IF)
        {
            new_branch_block = parse_branch_block(pc);
            branch_it->branch_expr.else_block = new_branch_block;
            branch_it = branch_it->branch_expr.else_block;
        }
        else
        {
            RED_UNREACHABLE;
            return null;
        }
    }
    while ((else_token = consume_token_if(pc, TOKEN_ID_KEYWORD_ELSE)));
        
    return branch_block;
}

static inline ASTNode* parse_return_statement(ParseContext* pc)
{
    Token* ret_token = consume_token_if(pc, TOKEN_ID_KEYWORD_RETURN);
    if (!ret_token)
    {
        return null;
    }
    ASTNode* node = NEW(ASTNode, 1);
    fill_base_node(node, ret_token, AST_TYPE_RETURN_STATEMENT);
    node->return_expr.expr = parse_expression(pc);
    expect_token(pc, TOKEN_ID_SEMICOLON);
    return node;
}

static inline ASTNode* parse_while_expr(ParseContext* pc)
{
    Token* token = expect_token(pc, TOKEN_ID_KEYWORD_WHILE);
    
    ASTNode* while_condition = parse_expression(pc);
    if (!while_condition)
    {
        os_exit_with_message("Expected expression after while statement\n");
        return null;
    }

    ASTNode* while_block = parse_expression(pc);
    if (!while_block)
    {
        os_exit_with_message("Expected block after while statement and condition\n");
        return null;
    }

    ASTNode* while_node = NEW(ASTNode, 1);
    fill_base_node(while_node, token, AST_TYPE_LOOP_EXPR);
    while_node->loop_expr.condition = while_condition;
    while_node->loop_expr.body = while_block;
    return while_node;
}

static inline ASTNode* parse_fn_call_expr(ParseContext* pc)
{
    Token* fn_expr_token = get_token(pc);
    if (fn_expr_token->id != TOKEN_ID_SYMBOL || get_token_i(pc, 1)->id != TOKEN_ID_LEFT_PARENTHESIS)
    {
        return null;
    }
    consume_token(pc);
    consume_token(pc);
    // TODO: modify to admit arguments
    ASTNode* param_arr[256];
    u8 param_count = 0;
    while (get_token(pc)->id != TOKEN_ID_RIGHT_PARENTHESIS)
    {
        param_arr[param_count] = parse_expression(pc);
        param_count++;
    }
    expect_token(pc, TOKEN_ID_RIGHT_PARENTHESIS);
    ASTNode* node = NEW(ASTNode, 1);
    fill_base_node(node, fn_expr_token, AST_TYPE_FN_CALL);
    node->fn_call.name = *token_buffer(fn_expr_token);
    node->fn_call.args = NEW(ASTNode*, param_count);
    memcpy(node->fn_call.args, param_arr, sizeof(ASTNode*) * param_count);
    node->fn_call.arg_count = param_count;
    return node;
}

static inline ASTNode* parse_array_literal(ParseContext* pc)
{
    Token* left_bracket = expect_token(pc, TOKEN_ID_LEFT_BRACKET);
    ASTNodeBuffer node_buffer = ZERO_INIT;
    ASTNode* value;
    while (get_token(pc)->id != TOKEN_ID_RIGHT_BRACKET && (value = parse_expression(pc)))
    {
        node_append(&node_buffer, value);
        expect_token_if_not(pc, TOKEN_ID_COMMA, TOKEN_ID_RIGHT_BRACKET);
    }
    expect_token(pc, TOKEN_ID_RIGHT_BRACKET);

    ASTNode* node = NEW(ASTNode, 1);
    fill_base_node(node, left_bracket, AST_TYPE_ARRAY_LIT);
    node->array_lit.values = node_buffer;

    return node;
}

static inline ASTNode* parse_primary_expr(ParseContext* pc)
{
    Token* t = get_token(pc);
    TokenID id = t->id;
    switch (id)
    {
        case TOKEN_ID_LEFT_BRACKET:
            return parse_array_literal(pc);
        case TOKEN_ID_LEFT_BRACE:
            return parse_compound_st(pc);
        case TOKEN_ID_LEFT_PARENTHESIS:
        {
            expect_token(pc, TOKEN_ID_LEFT_PARENTHESIS);
            ASTNode* node = parse_expression(pc);
            expect_token(pc, TOKEN_ID_RIGHT_PARENTHESIS);
            return node;
        }
        case TOKEN_ID_KEYWORD_IF:
            return parse_branch_block(pc);
        case TOKEN_ID_KEYWORD_WHILE:
            return parse_while_expr(pc);
        case TOKEN_ID_KEYWORD_RETURN:
            return parse_return_statement(pc);
        case TOKEN_ID_INT_LIT:
            return parse_int_literal(pc);
        case TOKEN_ID_SYMBOL:
            // TODO: fix
            if (get_token_i(pc, 1)->id == TOKEN_ID_LEFT_PARENTHESIS)
            {
                return parse_fn_call_expr(pc);
            }
            else
            {
                return parse_symbol_expr(pc);
            }
        case TOKEN_ID_END_OF_FILE:
            return null;
        default:
            RED_NOT_IMPLEMENTED;
            return null;
    }
}

static inline ASTNode* parse_right_expr(ParseContext* pc, ASTNode** left_expr)
{
    while (true)
    {
        Token* token = get_token(pc);
        if (token_is_binop_char(token->id))
        {
            consume_token(pc);
        }
        else
        {
            return *left_expr;
        }

        ASTNode* right_expr = parse_expression(pc);
        if (!right_expr)
        {
            return null;
        }
        
        ASTNode* node = NEW(ASTNode, 1);
        copy_base_node(node, *left_expr, AST_TYPE_BIN_EXPR);
        node->bin_expr.op = token->id;
        node->bin_expr.left = *left_expr;
        node->bin_expr.right = right_expr;
        *left_expr = node;
    }
}

//static inline ASTNode* parse_sym_expr(ParseContext* pc)
//{
//    Token* t = consume_token_if(pc, TOKEN_ID_SYMBOL);
//    if ()
//}

static inline ASTNode* parse_expression(ParseContext* pc)
{
    ASTNode* left_expr = parse_primary_expr(pc);
    if (!left_expr)
    {
        return null;
    }

    return parse_right_expr(pc, &left_expr);
}

static inline ASTNode* parse_fn_call_statement(ParseContext* pc)
{
    ASTNode* node = parse_fn_call_expr(pc);
    if (!node)
    {
        return null;
    }
    expect_token(pc, TOKEN_ID_SEMICOLON);
    return node;
}

static inline ASTNode* parse_statement(ParseContext* pc)
{
    ASTNode* node = parse_sym_decl(pc);
    if (node)
    {
        return node;
    }

    node = parse_fn_call_statement(pc);
    if (node)
    {
        return node;
    }

    node = parse_return_statement(pc);
    if (node)
    {
        return node;
    }

    node = parse_expression(pc);
    if (node)
    {
        bool add_semicolon = node->node_id != AST_TYPE_BRANCH_EXPR && node->node_id != AST_TYPE_COMPOUND_STATEMENT && node->node_id != AST_TYPE_LOOP_EXPR;
        if (add_semicolon)
        {
            expect_token(pc, TOKEN_ID_SEMICOLON);
        }
        return node;
    }

    RED_NOT_IMPLEMENTED;
    return null;
}
static inline ASTNode* parse_compound_st(ParseContext* pc)
{
    Token* start_block = consume_token_if(pc, TOKEN_ID_LEFT_BRACE);
    if (!start_block)
    {
        return nullptr;
    }

    ASTNode* block = NEW(ASTNode, 1);
    fill_base_node(block, start_block, AST_TYPE_COMPOUND_STATEMENT);

    // Empty blocks are not allowed
    if (get_token(pc)->id == TOKEN_ID_RIGHT_BRACE)
    {
        return nullptr;
    }

    do
    {
        ASTNode* statement = parse_statement(pc);
        if (statement)
        {
            node_append(&block->compound_statement.statements, statement);
        }
        else
        {
            os_exit(1);
        }
    } while (get_token(pc)->id != TOKEN_ID_RIGHT_BRACE);

    expect_token(pc, TOKEN_ID_RIGHT_BRACE);
    return block;
}

static inline ASTNode* parse_fn_proto(ParseContext* pc)
{
    Token* identifier = get_token(pc);
    if (identifier->id != TOKEN_ID_SYMBOL)
    {
        print("expected identifier for function prototype, found: %s\n", token_name(identifier->id));
        return null;
    }

    Token* eq_sign = get_token_i(pc, 1);
    if (eq_sign->id != TOKEN_ID_EQ)
    {
        print("Expected %s token for function prototype, found: %s\n", token_name(TOKEN_ID_EQ), token_name(eq_sign->id));
        return nullptr;
    }

    Token* left_parenthesis = get_token_i(pc, 2);
    if (left_parenthesis->id != TOKEN_ID_LEFT_PARENTHESIS)
    {
        print("Expected %s token for function prototype, found: %s\n", token_name(TOKEN_ID_LEFT_PARENTHESIS), token_name(left_parenthesis->id));
        return nullptr;
    }
    // name
    consume_token(pc);
    // equal
    consume_token(pc);
    consume_token(pc);

    ASTNodeBuffer param_list = parse_param_decl_list(pc);

    Token* return_type = get_token(pc);
    if (!return_type && (!(get_token(pc)->id == TOKEN_ID_SEMICOLON || get_token(pc)->id == TOKEN_ID_LEFT_BRACE)))
    {
        invalid_token_error(pc, get_token(pc));
    }

    ASTNode* return_type_node = NULL;
    if (return_type->id != TOKEN_ID_LEFT_BRACE)
    {
        return_type_node = create_type_node(pc);
    }

    ASTNode* node = NEW(ASTNode, 1);
    fill_base_node(node, identifier, AST_TYPE_FN_PROTO);
    node->fn_proto.params = param_list;
    node->fn_proto.sym = create_symbol_node(identifier);
    node->fn_proto.ret_type = return_type_node;
    return node;
}

static inline ASTNode* parse_fn_definition(ParseContext* pc)
{
    ASTNode* proto = parse_fn_proto(pc);
    if (!proto)
    {
        print("Error parsing function prototype for function (token %zu)\n", pc->current_token);
        return null;
    }

    ASTNode* body = parse_compound_st(pc);
    if (!body)
    {
        print("Error parsing function %s body\n", sb_ptr(proto->fn_proto.sym->sym_expr.name));
        return null;
    }

    ASTNode* fn_def = NEW(ASTNode, 1);
    copy_base_node(fn_def, proto, AST_TYPE_FN_DEF);
    fn_def->fn_def.proto = proto;
    fn_def->fn_def.body = body;

    return fn_def;
}

static inline bool is_complex_type_start(ParseContext* pc, TokenID container_type)
{
    Token* sym_name = get_token(pc);
    if (sym_name->id != TOKEN_ID_SYMBOL)
    {
        return false;
    }
    Token* eq = get_token_i(pc, 1);
    if (eq->id != TOKEN_ID_EQ)
    {
        return false;
    }

    Token* struct_tok = get_token_i(pc, 2);
    if (struct_tok->id != container_type)
    {
        return false;
    }

    return true;
}

static inline ASTNode* parse_container_field(ParseContext* pc)
{
    Token* name = consume_token_if(pc, TOKEN_ID_SYMBOL);
    if (!name)
    {
        return null;
    }

    ASTNode* node = NEW(ASTNode, 1);
    fill_base_node(node, name, AST_TYPE_FIELD_DECL);
    node->field_decl.sym = create_symbol_node(name);
    node->field_decl.type = create_type_node(pc);

    return node;
}

static inline ASTNodeBuffer parse_container_fields(ParseContext* pc)
{
    ASTNodeBuffer fields = ZERO_INIT;
    if (!consume_token_if(pc, TOKEN_ID_LEFT_BRACE))
    {
        os_exit_with_message("Expected opening brace in container declaration");
    }

    do
    {
        ASTNode* field = parse_container_field(pc);
        if (field)
        {
            node_append(&fields, field);
            expect_token(pc, TOKEN_ID_SEMICOLON);
        }
        else
        {
            os_exit_with_message("Can't parse field");
        }
    } while (get_token(pc)->id != TOKEN_ID_RIGHT_BRACE);

    expect_token(pc, TOKEN_ID_RIGHT_BRACE);

    return fields;
}

static inline ASTNode* parse_enum_field(ParseContext* pc, u32 count, bool* parse_enum_value)
{
    Token* name = consume_token_if(pc, TOKEN_ID_SYMBOL);
    if (!name)
    {
        return null;
    }

    ASTNode* value = null;
    bool parsing_enum_value = (bool)consume_token_if(pc, TOKEN_ID_EQ);
    if (parsing_enum_value)
    {
        if (count > 0 && !(*parse_enum_value))
        {
            os_exit_with_message("Enum coherence missing");
            return null;
        }
        value = parse_expression(pc);
    }
    else
    {
        if (count > 0 && (*parse_enum_value))
        {
            os_exit_with_message("Enum coherence missing");
            return null;
        }
    }

    *parse_enum_value = parsing_enum_value;

    ASTNode* node = NEW(Node, 1);
    fill_base_node(node, name, AST_TYPE_ENUM_DECL);
    node->enum_field.name = token_buffer(name);
    node->enum_field.field_value = value;

    return node;
}

static inline ASTNodeBuffer parse_enum_fields(ParseContext* pc)
{
    ASTNodeBuffer fields = ZERO_INIT;
    if (!consume_token_if(pc, TOKEN_ID_LEFT_BRACE))
    {
        os_exit_with_message("Expected opening brace in container declaration");
    }

    u32 count = 0;
    bool parse_enum_value = false;
    do
    {
        ASTNode* field = parse_enum_field(pc, count, &parse_enum_value);
        if (field)
        {
            count++;
            node_append(&fields, field);
            expect_token(pc, TOKEN_ID_SEMICOLON);
        }
        else
        {
            os_exit_with_message("Can't parse field");
        }
    } while (get_token(pc)->id != TOKEN_ID_RIGHT_BRACE);

    expect_token(pc, TOKEN_ID_RIGHT_BRACE);

    return fields;
}

static inline ASTNode* parse_struct_decl(ParseContext* pc)
{
    if (!is_complex_type_start(pc, TOKEN_ID_KEYWORD_STRUCT))
    {
        return null;
    }

    Token* first = consume_token(pc);
    consume_token(pc);
    consume_token(pc);

    ASTNode* node = NEW(ASTNode, 1);
    fill_base_node(node, first, AST_TYPE_STRUCT_DECL);
    node->struct_decl.fields = parse_container_fields(pc);
    node->struct_decl.name = first->str_lit.str;

    return node;
}

static inline ASTNode* parse_enum_decl(ParseContext* pc)
{
    if (!is_complex_type_start(pc, TOKEN_ID_KEYWORD_ENUM))
    {
        return null;
    }

    Token* name = expect_token(pc, TOKEN_ID_SYMBOL);
    expect_token(pc, TOKEN_ID_EQ);
    expect_token(pc, TOKEN_ID_KEYWORD_ENUM);

    ASTNode* node = NEW(Node, 1);
    fill_base_node(node, name, AST_TYPE_ENUM_DECL);
    // default enums are 32-bit
    node->enum_decl.type = ENUM_TYPE_U32;
    node->enum_decl.name = token_buffer(name);
    node->enum_decl.fields = parse_enum_fields(pc);

    return node;
}

bool parse_top_level_declaration(ParseContext* pc, RedAST* module_ast)
{
    ASTNode* node;

    if ((node = parse_struct_decl(pc)))
    {
        node_append(&module_ast->struct_decls, node);
        return true;
    }

    if ((node = parse_enum_decl(pc)))
    {
        node_append(&module_ast->enum_decls, node);
        return true;
    }

    if ((node = parse_fn_definition(pc)))
    {
        node_append(&module_ast->fn_definitions, node);
        return true;
    }

    return false;
}

RedAST parse_translation_unit(StringBuffer* src_buffer, TokenBuffer* tb)
{
    ParseContext pc = ZERO_INIT;
    pc.src_buffer = src_buffer;
    pc.token_buffer = tb;

    RedAST module_ast = ZERO_INIT;

    while ((get_token(&pc))->id != TOKEN_ID_END_OF_FILE)
    {
        bool result = parse_top_level_declaration(&pc, &module_ast);
        if (!result)
        {
            os_exit(1);
        }
    }

    return module_ast;
}

