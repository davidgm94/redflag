#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/DebugInfo.h>
#include <llvm-c/Transforms/PassManagerBuilder.h>
#include <llvm-c/Transforms/InstCombine.h>
#include <llvm-c/Transforms/Vectorize.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Transforms/IPO.h>
#include <llvm-c/Transforms/Utils.h>
#include <llvm-c/Comdat.h>

typedef struct LLVMContext
{
    LLVMContextRef context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    /// End general context
    ///
    /// </summary>
    LLVMValueRef function;
    LLVMValueRef alloca_point;
    Node* current_fn_decl;
    LLVMBasicBlockRef current_block;

    bool current_block_is_target;
    LLVMBasicBlockRef expr_block_exit;
    LLVMValueRef return_out;
} LLVMContext;

typedef struct LLVMSetup
{
    LLVMContextRef context;
    LLVMTargetRef target;
    char* default_target_triple;
    LLVMTargetMachineRef target_machine;
    LLVMTargetDataRef target_data_layout;
} LLVMSetup;

LLVMSetup llvm_setup(LLVMContext* ctx);
LLVMModuleRef llvm_setup_module(const char* module_name, const char* full_path, LLVMSetup* llvm_setup);

void llvm_codegen(RedModuleTree* mod_ast);
