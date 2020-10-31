#pragma once
#include "compiler_types.h"
#include "ast_types.h"
List<RedAST::Function*> parse(Buffer* file_buffer, List<Token>* tokens);

void parser_print_ast(List<RedAST::Function*>* fn_list);
