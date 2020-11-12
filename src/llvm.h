#pragma once

typedef struct RedModuleIR RedModuleIR;
void llvm_gen_machine_code(RedModuleIR* ir_tree);