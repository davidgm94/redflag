//
// Created by david on 7/10/20.
//

#ifndef REDFLAG_LEXER_H
#define REDFLAG_LEXER_H


#include "compiler_types.h"

#define BUFFER_TOKEN_FORMAT(token, symbol_name) StringBuffer* symbol_name = token->id == TOKEN_ID_SYMBOL ? token_buffer(token) : NULL
#define TOKEN_FORMAT(token_index, token, sn) " token #%zu at line %zu, column %zu: %s ... Name: %s\n", token_index, token->start_line + 1, token->start_column + 1, token_name(token->id), sn ? sn->ptr : "not a symbol"
#define PRINT_TOKEN_WITH_PREFIX(prefix_str, token, token_index, symbol_name) BUFFER_TOKEN_FORMAT(token, symbol_name); print(prefix_str TOKEN_FORMAT(token_index, token, symbol_name))

static inline BigInt* token_bigint(Token* token)
{
    if (!token)
    {
        return NULL;
    }
    redassert(token->id == TOKEN_ID_INT_LIT);
    return &token->int_lit.big_int;
}

static inline BigFloat* token_bigfloat(Token* token)
{
    if (!token)
    {
        return NULL;
    }
    redassert(token->id == TOKEN_ID_FLOAT_LIT);
    return &token->float_lit.big_float;
}

static inline bool token_is_binop_char(TokenID op)
{
    bool is_it = op == TOKEN_ID_PLUS ||
                 op == TOKEN_ID_EQ ||
                 op == TOKEN_ID_DASH ||
                 op == TOKEN_ID_STAR ||
                 op == TOKEN_ID_SLASH ||
                 op == TOKEN_ID_CMP_NOT_EQ ||
                 op == TOKEN_ID_CMP_EQ ||
                 op == TOKEN_ID_CMP_GREATER ||
                 op == TOKEN_ID_CMP_GREATER_OR_EQ ||
                 op == TOKEN_ID_CMP_LESS ||
                 op == TOKEN_ID_CMP_LESS_OR_EQ ||
                 op == TOKEN_ID_KEYWORD_OR ||
                 op == TOKEN_ID_KEYWORD_AND;
    return is_it;
}
static inline StringBuffer* token_buffer(Token* token)
{
    if (!token)
    {
        return NULL;
    }
    redassert(token->id == TOKEN_ID_STRING_LIT || token->id == TOKEN_ID_MULTILINE_STRING_LIT || token->id == TOKEN_ID_SYMBOL || token->id == TOKEN_ID_KEYWORD_RAW_STRING);
    return &token->str_lit.str;
}

LexingResult lex_file(SB* src_buffer);
void print_tokens(SB* src_buffer, TokenBuffer* tokens);
const char* token_name(TokenID token_enum);
bool valid_symbol_starter(char c);
bool is_red_keyword_sb(SB* src_buffer);
bool is_red_keyword_id(TokenID token_id);
void token_buffer_append_buffer(TokenBuffer* dst, TokenBuffer* src);
void add_module(SB* module_file);

#endif //REDFLAG_LEXER_H
