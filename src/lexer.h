//
// Created by david on 7/10/20.
//

#ifndef REDFLAG_LEXER_H
#define REDFLAG_LEXER_H


#include "compiler_types.h"

#define BUFFER_TOKEN_FORMAT(token, symbol_name) StringBuffer* symbol_name = token->id == TOKEN_ID_SYMBOL ? token_buffer(token) : NULL
#define TOKEN_FORMAT(token_index, token, sn) " token #%zu at line %zu, column %zu: %s ... Name: %s\n", token_index, token->start_line + 1, token->start_column + 1, token_name(token->id), sn ? sn->ptr : "not a symbol"
#define PRINT_TOKEN_WITH_PREFIX(prefix_str, token, token_index, symbol_name) BUFFER_TOKEN_FORMAT(token, symbol_name); print(prefix_str TOKEN_FORMAT(token_index, token, symbol_name))

LexingResult lex_file(SB* src_buffer);
void print_tokens(SB* src_buffer, TokenBuffer* tokens);
const char* token_name(TokenID token_enum);
bool valid_symbol_starter(char c);
bool is_red_keyword_sb(SB* src_buffer);
bool is_red_keyword_id(TokenID token_id);

#endif //REDFLAG_LEXER_H
