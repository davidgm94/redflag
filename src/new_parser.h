#pragma once
#include "compiler_types.h"
List<RedAST::Function*> new_parse(Buffer* file_buffer, List<Token>* tokens);
