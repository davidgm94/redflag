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


static inline void copy_base_node(Node* dst, const Node* src, ASTType id)
{
    dst->node_id = id;
    dst->node_line = src->node_line;
    dst->node_column = src->node_column;
    static_assert(offsetof(Node, sym_decl) == (sizeof(dst->node_id) + sizeof(dst->node_line) + sizeof(dst->node_column) + sizeof(dst->node_padding)), "Base node size has been incorrectly modified");
}

static inline void fill_base_node(Node* bn, Token* t, ASTType id)
{
    bn->node_id = id;
    bn->node_line = t->start_line;
    bn->node_column = t->start_column;
}

static inline ASTType get_node_type(Node* n)
{
    return n->node_id;
}

static inline Node* create_symbol_node(Token* t)
{
    Node* node = NEW(Node, 1);
    fill_base_node(node, t, AST_TYPE_SYM_EXPR);
    node->sym_expr.name = &t->str_lit.str;
    return node;
}

static inline Node* create_type_node(Token* t)
{
    if (t)
    {
        Node* node = NEW(Node, 1);
        fill_base_node(node, t, AST_TYPE_TYPE_EXPR);
        node->type_expr.type_name = &t->str_lit.str;
        return node;
    }

    return null;
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

static inline Node* parse_expression(ParseContext* pc);
static inline Node* parse_param_decl(ParseContext* pc)
{
    Token* name = expect_token(pc, TOKEN_ID_SYMBOL);
    Token* type = expect_token(pc, TOKEN_ID_SYMBOL);
    if (get_token(pc)->id != TOKEN_ID_RIGHT_PARENTHESIS)
    {
        expect_token(pc, TOKEN_ID_COMMA);
    }

    Node* param = NEW(Node, 1);
    fill_base_node(param, name, AST_TYPE_PARAM_DECL);
    param->param_decl.sym = create_symbol_node(name);
    param->param_decl.type = create_type_node(type);
    return param;
}

static inline NodeBuffer parse_param_decl_list(ParseContext* pc)
{
    // Left parenthesis is already consumed
    //expect_token(pc, TOKEN_ID_LEFT_PARENTHESIS);

    NodeBuffer nb = ZERO_INIT;
    while (get_token(pc)->id != TOKEN_ID_RIGHT_PARENTHESIS)
    {
        Node* param = parse_param_decl(pc);
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

static inline Node* parse_sym_decl(ParseContext* pc)
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
    Token* sym_type = expect_token(pc, TOKEN_ID_SYMBOL);

    // TODO: This means no value assigned, uninitialized (left to the backend?????)
    Node* sym_node = null;
    Token* semicolon = consume_token_if(pc, TOKEN_ID_SEMICOLON);
    if (semicolon)
    {
        sym_node = NEW(Node, 1);
        fill_base_node(sym_node, mut_token, AST_TYPE_SYM_DECL);
        sym_node->sym_decl.is_const = is_const;
        sym_node->sym_decl.sym = create_symbol_node(sym_name);
        sym_node->sym_decl.type = create_type_node(sym_type);
        return sym_node;
    }

    expect_token(pc, TOKEN_ID_EQ);
    Node* expression = parse_expression(pc);
    expect_token(pc, TOKEN_ID_SEMICOLON);

    sym_node = NEW(Node, 1);
    fill_base_node(sym_node, mut_token, AST_TYPE_SYM_DECL);
    sym_node->sym_decl.is_const = is_const;
    sym_node->sym_decl.sym = create_symbol_node(sym_name);
    sym_node->sym_decl.type = create_type_node(sym_type);
    sym_node->sym_decl.fn_handle = expression;

    return sym_node;
}

/*
Var declaration
Branch (if-else)
return expression
variable assignment (bin op?)
*/
static inline Node* parse_int_literal(ParseContext* pc)
{
    Token* token = consume_token_if(pc, TOKEN_ID_INT_LIT);
    if (!token)
    {
        return null;
    }

    Node* node = NEW(Node, 1);
    fill_base_node(node, token, AST_TYPE_INT_LIT);
    node->int_lit.bigint = token_bigint(token);

    return node;
}

static inline Node* parse_symbol_expr(ParseContext* pc)
{
    Token* token = consume_token_if(pc, TOKEN_ID_SYMBOL);
    if (!token)
    {
        return null;
    }

    Node* node = create_symbol_node(token);
    return node;
}

static inline Node* parse_primary_expr(ParseContext* pc);
static inline Node* parse_branch_block(ParseContext* pc)
{
    Token* if_token = expect_token(pc, TOKEN_ID_KEYWORD_IF);

    Node* condition_node = parse_expression(pc);
    if (!condition_node)
    {
        return null;
    }

    Node* if_block = parse_expression(pc);
    if (!if_block)
    {
        RED_NOT_IMPLEMENTED;
        return null;
    }

    Token* else_token = consume_token_if(pc, TOKEN_ID_KEYWORD_ELSE);
    if (!else_token)
    {
        Node* branch_block = NEW(Node, 1);
        fill_base_node(branch_block, if_token, AST_TYPE_BRANCH_EXPR);
        branch_block->branch_expr.condition = condition_node;
        branch_block->branch_expr.if_block = if_block;
        branch_block->branch_expr.else_block = null;

        return branch_block;
    }

    Token* else_if_token = consume_token_if(pc, TOKEN_ID_KEYWORD_IF);
    if (!else_if_token)
    {
        // IF-ELSE BLOCK

        // parse else block
        Node* else_block = parse_expression(pc);
        if (!else_block)
        {
            RED_NOT_IMPLEMENTED;
            return null;
        }
        redassert(else_block->node_id == AST_TYPE_COMPOUND_STATEMENT);

        Node* branch_block = NEW(Node, 1);
        fill_base_node(branch_block, if_token, AST_TYPE_BRANCH_EXPR);
        branch_block->branch_expr.condition = condition_node;
        branch_block->branch_expr.if_block = if_block;
        branch_block->branch_expr.else_block = else_block;

        return branch_block;
    }

    while ((else_token = consume_token_if(pc, TOKEN_ID_KEYWORD_ELSE)))
    {
        else_if_token = consume_token_if(pc, TOKEN_ID_KEYWORD_IF);
        if (else_if_token)
        {

        }
    }
        
    RED_NOT_IMPLEMENTED;
    return null;
}

static inline Node* parse_return_statement(ParseContext* pc)
{
    Token* ret_token = consume_token_if(pc, TOKEN_ID_KEYWORD_RETURN);
    if (!ret_token)
    {
        return null;
    }
    Node* node = NEW(Node, 1);
    fill_base_node(node, ret_token, AST_TYPE_RETURN_STATEMENT);
    node->return_expr.expr = parse_expression(pc);
    expect_token(pc, TOKEN_ID_SEMICOLON);
    return node;
}

static inline Node* parse_compound_st(ParseContext* pc);
static inline Node* parse_primary_expr(ParseContext* pc)
{
    Token* t = get_token(pc);
    TokenID id = t->id;
    switch (id)
    {
        case TOKEN_ID_LEFT_BRACE:
            return parse_compound_st(pc);
        case TOKEN_ID_LEFT_PARENTHESIS:
        {
            expect_token(pc, TOKEN_ID_LEFT_PARENTHESIS);
            Node* node = parse_expression(pc);
            expect_token(pc, TOKEN_ID_RIGHT_PARENTHESIS);
            return node;
        }
        case TOKEN_ID_KEYWORD_IF:
            return parse_branch_block(pc);
        case TOKEN_ID_KEYWORD_RETURN:
            return parse_return_statement(pc);
        case TOKEN_ID_INT_LIT:
            return parse_int_literal(pc);
        case TOKEN_ID_SYMBOL:
            return parse_symbol_expr(pc);
        case TOKEN_ID_END_OF_FILE:
            return null;
        default:
            RED_NOT_IMPLEMENTED;
            return null;
    }
}

static inline Node* parse_right_expr(ParseContext* pc, Node** left_expr)
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

        Node* right_expr = parse_expression(pc);
        if (!right_expr)
        {
            return null;
        }
        
        Node* node = NEW(Node, 1);
        copy_base_node(node, *left_expr, AST_TYPE_BIN_EXPR);
        node->bin_expr.op = token->id;
        node->bin_expr.left = *left_expr;
        node->bin_expr.right = right_expr;
        *left_expr = node;
    }
}

//static inline Node* parse_sym_expr(ParseContext* pc)
//{
//    Token* t = consume_token_if(pc, TOKEN_ID_SYMBOL);
//    if ()
//}

static inline Node* parse_expression(ParseContext* pc)
{
    Node* left_expr = parse_primary_expr(pc);
    if (!left_expr)
    {
        return null;
    }

    if (left_expr->node_id == AST_TYPE_BIN_EXPR && left_expr->bin_expr.op == TOKEN_ID_EQ)
    {
        int k = 125123124;
    }

    return parse_right_expr(pc, &left_expr);
}

static inline Node* parse_statement(ParseContext* pc)
{
    Node* node = parse_sym_decl(pc);
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
        bool add_semicolon = node->node_id != AST_TYPE_BRANCH_EXPR && node->node_id != AST_TYPE_COMPOUND_STATEMENT;
        if (add_semicolon)
        {
            expect_token(pc, TOKEN_ID_SEMICOLON);
        }
        return node;
    }

    RED_NOT_IMPLEMENTED;
    return null;
}
static inline Node* parse_compound_st(ParseContext* pc)
{
    Token* start_block = consume_token_if(pc, TOKEN_ID_LEFT_BRACE);
    if (!start_block)
    {
        return nullptr;
    }

    Node* block = NEW(Node, 1);
    fill_base_node(block, start_block, AST_TYPE_COMPOUND_STATEMENT);

    // Empty blocks are not allowed
    if (get_token(pc)->id == TOKEN_ID_RIGHT_BRACE)
    {
        return nullptr;
    }

    do
    {
        Node* statement = parse_statement(pc);
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

static inline Node* parse_fn_proto(ParseContext* pc)
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

    NodeBuffer param_list = parse_param_decl_list(pc);

    Token* return_type = consume_token_if(pc, TOKEN_ID_SYMBOL);
    if (!return_type && (!(get_token(pc)->id == TOKEN_ID_SEMICOLON || get_token(pc)->id == TOKEN_ID_LEFT_BRACE)))
    {
        invalid_token_error(pc, get_token(pc));
    }

    Node* node = NEW(Node, 1);
    fill_base_node(node, identifier, AST_TYPE_FN_PROTO);
    node->fn_proto.params = param_list;
    node->fn_proto.sym = create_symbol_node(identifier);
    node->fn_proto.ret_type = create_type_node(return_type);
    return node;
}

static inline Node* parse_fn_definition(ParseContext* pc)
{
    Node* proto = parse_fn_proto(pc);
    if (!proto)
    {
        print("Error parsing function prototype for function (token %zu)\n", pc->current_token);
        return null;
    }

    Node* body = parse_compound_st(pc);
    if (!body)
    {
        print("Error parsing function %s body\n", sb_ptr(proto->fn_proto.sym->sym_expr.name));
        return null;
    }

    Node* fn_def = NEW(Node, 1);
    copy_base_node(fn_def, proto, AST_TYPE_FN_DEF);
    fn_def->fn_def.proto = proto;
    fn_def->fn_def.body = body;

    return fn_def;
}

bool parse_top_level_declaration(ParseContext* pc, RedModuleTree* module_ast)
{
    Node* node;
    if ((node = parse_fn_definition(pc)))
    {
        node_append(&module_ast->fn_definitions, node);
        return true;
    }

    return false;
}

RedModuleTree parse_translation_unit(StringBuffer* src_buffer, TokenBuffer* tb)
{
    ParseContext pc = ZERO_INIT;
    pc.src_buffer = src_buffer;
    pc.token_buffer = tb;

    RedModuleTree module_ast = ZERO_INIT;

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

