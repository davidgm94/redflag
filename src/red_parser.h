//
// Created by david on 10/18/20.
//

#pragma once

#if NEW_PARSER == 0
#include "compiler_types.h"

ASTNode* red_parse(Buffer* buffer, List<Token>* tokens, RedType* owner);
void red_parser_print(ASTNode* node, s32 indent);
void red_parser_visit_node_children(ASTNode* node, void (*visit)(ASTNode**, void* context), void* context);

#endif
