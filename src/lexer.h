//
// Created by david on 7/10/20.
//

#ifndef REDFLAG_LEXER_H
#define REDFLAG_LEXER_H


#include "compiler_types.h"

#define BUFFER_TOKEN_FORMAT(token, symbol_name) Buffer* symbol_name = token->id == TOKEN_ID_SYMBOL ? token_buffer(token) : nullptr
#define TOKEN_FORMAT(token, sn) " token #%zu at line %zu, column %zu: %s ... Name: %s\n", pc->current_token, token->start_line + 1, token->start_column + 1, token_name(token->id), sn ? sn->items : "not a symbol"
#define PRINT_TOKEN_WITH_PREFIX(prefix_str, token, symbol_name) BUFFER_TOKEN_FORMAT(token, symbol_name); print(prefix_str TOKEN_FORMAT(token, symbol_name))

LexingResult lex(Buffer* buffer);
void print_tokens(Buffer* buffer, List<Token>* tokens);
const char* token_name(TokenID token_enum);
bool valid_symbol_starter(char c);
bool is_red_keyword(Buffer* buffer);
bool is_red_keyword(TokenID token_id);

#endif //REDFLAG_LEXER_H
