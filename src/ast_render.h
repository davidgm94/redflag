//
// Created by david on 10/16/20.
//

#pragma once

#include "types.h"
#include <stdio.h>
struct ASTNode;
void ast_print(FILE* file, ASTNode* node, s32 indent);
void ast_render(FILE* file, ASTNode* node, s32 indent_size);
