#include "llvm.h"
#include "compiler_types.h"
#include "ir.h"
#include "os.h"

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

static const u32 max_param_count = 100;

typedef struct RedLLVMFn
{
    LLVMTypeRef param_types[max_param_count];
    LLVMValueRef value;
    LLVMTypeRef type;
    LLVMTypeRef ret_type;
    u8 param_count;
} RedLLVMFn;


static inline LLVMTypeRef llvm_gen_type(LLVMContextRef context, RedType* type)
{
    if (type)
    {
        redassert(type->kind == PRIMITIVE);
        redassert(type->primitive == RED_TYPE_PRIMITIVE_S32);

        return LLVMInt32TypeInContext(context);
    }

    return LLVMVoidTypeInContext(context);
}

static inline LLVMTypeRef llvm_gen_fn_type(RedLLVMFn* fn_struct, LLVMContextRef context, IRFunctionPrototype* proto)
{
    fn_struct->ret_type = llvm_gen_type(context, &proto->ret_type);
    redassert(fn_struct->ret_type);

    fn_struct->param_count = proto->param_count;

    if (fn_struct->param_count > 0)
    {
        redassert(fn_struct->param_count < max_param_count);

        for (u32 i = 0; i < fn_struct->param_count; i++)
        {
            RedType* red_type = &proto->params[i].type;
            fn_struct->param_types[i] = llvm_gen_type(context, red_type);
            redassert(fn_struct->param_types[i]);
        }
    }

    fn_struct->type = LLVMFunctionType(fn_struct->ret_type, fn_struct->param_types, fn_struct->param_count, false);
    return fn_struct->type;
}

static inline RedLLVMFn llvm_gen_fn_proto(LLVMContextRef context, LLVMModuleRef module, IRFunctionPrototype* proto)
{
    RedLLVMFn fn_struct = ZERO_INIT;
    LLVMValueRef fn = LLVMAddFunction(module, sb_ptr(proto->name), llvm_gen_fn_type(&fn_struct, context, proto));
    LLVMSetFunctionCallConv(fn, LLVMCCallConv);
    LLVMSetLinkage(fn, LLVMExternalLinkage);
    LLVMSetVisibility(fn, LLVMDefaultVisibility);

    fn_struct.value = fn;

    return fn_struct;
}

static inline void llvm_verify_function(LLVMValueRef fn, const char* fn_type)
{
    usize str_len = 0;
    const char* fn_name = LLVMGetValueName2(fn, &str_len);
    if (LLVMVerifyFunction(fn, LLVMPrintMessageAction))
    {
        os_exit_with_message("%s FAIL\n", fn_name);
    }
    else
    {
        print("%s OK\n", fn_name);
    }
}

static inline void llvm_verify_module(LLVMModuleRef module)
{
#if 1
#if RED_LLVM_VERBOSE
    print("\n\n****\n");
    print("%s\n****\n\n", LLVMPrintModuleToString(module));
#endif
#endif
    char* error = NULL;
#if RED_LLVM_VERBOSE
    if (LLVMVerifyModule(module, LLVMPrintMessageAction, &error))
#else
    if (LLVMVerifyModule(module, LLVMReturnStatusAction, &error))
#endif
    {
#if RED_LLVM_VERBOSE
        os_exit_with_message("\nModule verified: %s\n\n", error);
#endif
    }
    else
    {
#if RED_LLVM_VERBOSE
        prints("\nVerified module\n");
#endif
    }
}

static inline LLVMValueRef llvm_gen_expression(LLVMContextRef context, LLVMBuilderRef builder, IRExpression* expression, IRFunctionDefinition* fn_definition, LLVMValueRef* params);









static inline LLVMValueRef llvm_gen_expression(LLVMContextRef context, LLVMBuilderRef builder, IRExpression* expression, IRFunctionDefinition* fn_definition, LLVMValueRef* params)
{
    IRExpressionType type = expression->type;
    switch (type)
    {
        case IR_EXPRESSION_TYPE_BIN_EXPR:
        {
            IRBinaryExpr* bin_expr = &expression->bin_expr;
            LLVMValueRef left = llvm_gen_expression(context, builder, bin_expr->left, fn_definition, params);
            LLVMValueRef right = llvm_gen_expression(context, builder, bin_expr->right, fn_definition, params);
            TokenID op = bin_expr->op;

            switch (op)
            {
                case TOKEN_ID_CMP_GREATER:
                    return LLVMBuildICmp(builder, LLVMIntSGT, left, right, "gt");
                case TOKEN_ID_CMP_EQ:
                    return LLVMBuildICmp(builder, LLVMIntEQ, left, right, "eq");
                case TOKEN_ID_PLUS:
                    return LLVMBuildAdd(builder, left, right, "add");
                case TOKEN_ID_DASH:
                    return LLVMBuildSub(builder, left, right, "sub");
                case TOKEN_ID_STAR:
                    return LLVMBuildMul(builder, left, right, "mul");
                case TOKEN_ID_SLASH:
                    // TODO: which division we want to perform
                    return LLVMBuildExactSDiv(builder, left, right, "exact_div");
                default:
                    RED_NOT_IMPLEMENTED;
                    break;
            }
            break;
        }
        case IR_EXPRESSION_TYPE_SYM_EXPR:
        {
            IRSymExpr* sym_expr = &expression->sym_expr;
            IRSymExprType se_type = sym_expr->type;
            switch (se_type)
            {
                case IR_SYM_EXPR_TYPE_PARAM:
                {
                    IRParamDecl* param = sym_expr->param_decl;
                    u8 index = (u8)(param - fn_definition->proto.params);
                    return LLVMBuildLoad(builder, params[index], sb_ptr(param->name));
                }
                default:
                    RED_NOT_IMPLEMENTED;
                    break;
            }
            break;
        }
        case IR_EXPRESSION_TYPE_INT_LIT:
        {
            IRIntLiteral* int_lit = &expression->int_literal;
            redassert(int_lit->bigint.digit_count == 1);
            u64 n = int_lit->bigint.digit;
            // TODO: fix type
            return LLVMConstInt(LLVMInt32TypeInContext(context), n, int_lit->bigint.is_negative);
        }
        default:
            RED_NOT_IMPLEMENTED;
            break;
    }
    return null;
}

static inline LLVMValueRef llvm_gen_statement(LLVMContextRef context, LLVMBuilderRef builder, IRStatement* st, IRFunctionDefinition* fn_definition, LLVMValueRef current_fn, LLVMValueRef* params);

static inline void llvm_gen_compound_statement(LLVMContextRef context, LLVMBuilderRef builder, IRCompoundStatement* compound_st, IRFunctionDefinition* fn_definition, LLVMValueRef current_fn, LLVMValueRef* params)
{
    IRStatementBuffer* st_bf = &compound_st->stmts;
    IRStatement* st_it = st_bf->ptr;
    u32 st_count = st_bf->len;
    if (st_count > 0)
    {
        for (u32 i = 0; i < st_count; i++)
        {
            IRStatement* st = &st_it[i];
            llvm_gen_statement(context, builder, st, fn_definition, current_fn, params);
        }
    }
    else
    {
        RED_UNREACHABLE;
    }
}

static inline LLVMValueRef llvm_gen_statement(LLVMContextRef context, LLVMBuilderRef builder, IRStatement* st, IRFunctionDefinition* fn_definition, LLVMValueRef current_fn, LLVMValueRef* params)
{
    IRStatementType type = st->type;
    switch (type)
    {
        case IR_ST_TYPE_RETURN_ST:
        {
            IRReturnStatement* ret_st = &st->return_st;
            IRExpressionType expr_type = ret_st->expression.type;
            IRExpression* expr = &ret_st->expression;
            LLVMValueRef ret_value = llvm_gen_expression(context, builder, expr, fn_definition, params);
            redassert(ret_value);

            LLVMBuildRet(builder, ret_value);
            break;
        }
        case IR_ST_TYPE_BRANCH_ST:
        {
            IRBranchStatement* branch_st = &st->branch_st;

            LLVMBasicBlockRef if_block = LLVMAppendBasicBlockInContext(context, current_fn, "if_block");
            bool else_block_exists = branch_st->else_block.stmts.len > 0;
            LLVMBasicBlockRef else_block = null;
            LLVMBasicBlockRef branch_end_block = LLVMAppendBasicBlockInContext(context, current_fn, "if_cont");
            if (else_block_exists)
            {
                else_block = LLVMAppendBasicBlockInContext(context, current_fn, "else_block");
            }
            //else
            //{
            //    else_block = branch_end_block;
            //}

            LLVMValueRef condition_value = llvm_gen_expression(context, builder, &branch_st->condition, fn_definition, params);
            redassert(condition_value);

#define C3C 0
#if C3C == 0
            LLVMBuildCondBr(builder, condition_value, if_block, else_block);
            LLVMPositionBuilderAtEnd(builder, if_block);

            /* If block */
            IRCompoundStatement* ir_if_block = &branch_st->if_block;
            // WRONG: if statements should be generated not by compound statement gen function
            //LLVMValueRef llvm_if_block = llvm_gen_compound_statement(context, builder, if_block, fn_definition, params);

            LLVMAppendExistingBasicBlock(current_fn, if_block);
            LLVMPositionBuilderAtEnd(builder, if_block);

            // emit statement
            llvm_gen_compound_statement(context, builder, ir_if_block, fn_definition, current_fn, params);
            // end emit statement

            LLVMBuildBr(builder, branch_end_block);
            /* End If block */
            
            // TODO: else block optional. Make "else-if" happen
            /* Else block */
            IRCompoundStatement* ir_else_block = &branch_st->else_block;
            LLVMAppendExistingBasicBlock(current_fn, else_block);
            LLVMPositionBuilderAtEnd(builder, else_block);

            // emit statement
            llvm_gen_compound_statement(context, builder, ir_else_block, fn_definition, current_fn, params);
            // end emit statement

            LLVMBuildBr(builder, branch_end_block);

            /* End else block */

            LLVMAppendExistingBasicBlock(current_fn, branch_end_block);
            LLVMPositionBuilderAtEnd(builder, branch_end_block);
#else
            if (LLVMIsConstant(condition_value))
            {
                u32 br_v = LLVMConstIntGetZExtValue(condition_value);
                // TODO: dummy value
                LLVMBasicBlockRef dummy = NULL;
                if (br_v)
                {
                    llvm_gen_br(builder, &dummy, if_block);
                    else_block = null;
                }
                else
                {
                    llvm_gen_br(builder, &dummy, else_block);
                    if_block = null;
                }
            }
            else
            {
                LLVMBuildCondBr(builder, condition_value, if_block, else_block);
                LLVMClearInsertionPosition(builder);
                // context current block = null
            }
#endif

            return null;
        }
        case IR_ST_TYPE_SYM_DECL_ST:
            RED_NOT_IMPLEMENTED;
            return null;
        default:
            RED_NOT_IMPLEMENTED;
            break;
    }
}

static inline void llvm_gen_fn_definition(LLVMContextRef context, LLVMModuleRef module, IRFunctionDefinition* fn_definition)
{
    /* Prototype */
    RedLLVMFn fn_struct = llvm_gen_fn_proto(context, module, &fn_definition->proto);
    LLVMValueRef fn = fn_struct.value;

    /* Definition */
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(context, fn, "entry");
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);
    LLVMPositionBuilderAtEnd(builder, entry);

    u8 param_count = fn_definition->proto.param_count;
    LLVMValueRef* params = null;
    LLVMValueRef param_arr[max_param_count];
    LLVMValueRef param_alloc_arr[max_param_count];
    IRParamDecl* ir_params = fn_definition->proto.params;

    if (param_count > 0)
    {
        redassert(param_count < max_param_count);
        redassert(ir_params);
        params = param_arr;
        LLVMGetParams(fn, params);

        for (usize i = 0; i < param_count; i++)
        {
            LLVMSetValueName(params[i], ir_params[i].name->ptr);
            param_alloc_arr[i] = LLVMBuildAlloca(builder, LLVMTypeOf(params[i]), "");
        }
    }

    // TODO: improve this (stores might need to be emitted even if there's just a return statement (because it could be a complex expression)
#if 0
    bool emit_stores = param_count > 0 && param_count > 1;
#else
    bool emit_stores = param_count > 0;
#endif
    if (emit_stores)
    {
        for (usize i = 0; i < param_count; i++)
        {
            LLVMBuildStore(builder, params[i], param_alloc_arr[i]);
        }
    }

    u32 st_count = fn_definition->body.stmts.len;
    if (st_count > 0)
    {
        IRCompoundStatement* body = &fn_definition->body;
        llvm_gen_compound_statement(context, builder, body, fn_definition, fn, param_alloc_arr);
    }
    else if (st_count == 0)
    {
        LLVMValueRef ret_value = LLVMConstInt(LLVMInt32TypeInContext(context), 0, false);
        LLVMBuildRet(builder, ret_value);
    }
    else
    {
        RED_UNREACHABLE;
    }

#if RED_LLVM_VERBOSE
    llvm_verify_function(fn, "definition");
#endif
}

typedef struct RedLLVMContext
{
    LLVMContextRef context;
    LLVMTargetRef target;
    char* default_target_triple;
    LLVMTargetMachineRef target_machine;
    LLVMTargetDataRef target_data_layout;
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
} RedLLVMContext;

static inline RedLLVMContext llvm_init(void)
{
    RedLLVMContext ctx = ZERO_INIT;
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllTargets();
    LLVMInitializeAllAsmPrinters();
    LLVMInitializeAllAsmParsers();
    //LLVMInitializeAllDisassemblers();
    ctx.default_target_triple = LLVMGetDefaultTargetTriple();
    redassert(ctx.default_target_triple);
    char* error_message = null;
    if (LLVMGetTargetFromTriple(ctx.default_target_triple, &ctx.target, &error_message))
    {
        os_exit_with_message("Couldn't create target: %s\n", error_message);
    }
    redassert(ctx.target);

    s32 alloca_address_space = 0;
//#if RED_LLVM_VERBOSE
//    print("Target set to %s\n", ctx.default_target_triple);
//#endif

    LLVMCodeGenOptLevel opt_level = LLVMCodeGenLevelNone;
    LLVMRelocMode reloc_mode = LLVMRelocDefault;
    const char* cpu = "generic";

    ctx.target_machine = LLVMCreateTargetMachine(ctx.target, ctx.default_target_triple, "", "", opt_level, reloc_mode, LLVMCodeModelDefault);

    redassert(ctx.target_machine);

    if (!ctx.target_machine)
    {
        os_exit_with_message("Failed to create target machine\n");
    }

    ctx.target_data_layout = LLVMCreateTargetDataLayout(ctx.target_machine);
    redassert(ctx.target_data_layout);
    
    //char* target_triple = LLVMGetTargetMachineTriple(ctx.target_machine);

    //LLVMDisposeMessage(target_triple);
    ctx.context = LLVMContextCreate();

    return ctx;
}

static inline void llvm_setup_module(const char* module_name, const char* full_path, RedLLVMContext* ctx)
{
    ctx->module = LLVMModuleCreateWithNameInContext(module_name, ctx->context);
    LLVMSetModuleDataLayout(ctx->module, ctx->target_data_layout);
    LLVMSetSourceFileName(ctx->module, full_path, strlen(full_path));
    LLVMSetTarget(ctx->module, ctx->default_target_triple);
}

void llvm_gen_machine_code(RedModuleIR* ir_tree)
{
    RedLLVMContext ctx = llvm_init();
    llvm_setup_module("red_module", "whatever", &ctx);

    IRFunctionDefinitionBuffer* fn_defs = &ir_tree->fn_definitions;
    u32 fn_def_count = fn_defs->len;
    for (usize i = 0; i < fn_def_count; i++)
    {
        IRFunctionDefinition* fn_def_it = &fn_defs->ptr[i];
        llvm_gen_fn_definition(ctx.context, ctx.module, fn_def_it);
    }

    llvm_verify_module(ctx.module);
}
