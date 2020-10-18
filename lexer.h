//
// Created by david on 7/10/20.
//

#ifndef REDFLAG_LEXER_H
#define REDFLAG_LEXER_H


#include "compiler_types.h"


LexingResult lex(Buffer* buffer);
void print_tokens(Buffer* buffer, List<Token>* tokens);
const char* token_name(TokenID token_enum);
bool valid_symbol_starter(char c);
bool is_red_keyword(Buffer* buffer);

#endif //REDFLAG_LEXER_H
