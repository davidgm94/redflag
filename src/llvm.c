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

GEN_BUFFER_STRUCT(LLVMValueRef)
GEN_BUFFER_FUNCTIONS(alloca, ab, LLVMValueRefBuffer, LLVMValueRef)

typedef struct RedLLVMFn
{
    LLVMTypeRef param_types[max_param_count];
    LLVMValueRef param_arr[max_param_count];
    LLVMValueRef param_alloc_arr[max_param_count];
    LLVMValueRefBuffer alloca_buffer;
    LLVMValueRef fn_handle;
    LLVMTypeRef type;
    LLVMTypeRef ret_type;
    LLVMValueRef ret_alloca;
    u32 depth_level;
    bool return_already_emitted;
    u8 param_count;
} RedLLVMFn;

typedef struct RedLLVMContext
{
    LLVMContextRef context;
    LLVMBuilderRef builder;

    LLVMValueRef function;
    IRFunctionDefinition* current_fn;
    RedLLVMFn* llvm_current_fn;
    /* Not used
    LLVMValueRef alloca_point;
    LLVMBasicBlockRef current_block;

    bool current_block_is_target;
    LLVMBasicBlockRef expr_block_exit;
    LLVMValueRef return_out;
    */
    LLVMTargetRef target;
    char* default_target_triple;
    LLVMTargetMachineRef target_machine;
    LLVMTargetDataRef target_data_layout;
    LLVMModuleRef module;
} RedLLVMContext;

static inline void llvm_debug_fn(LLVMValueRef fn)
{
    print("Debugging function\n\n%s\n\n", LLVMPrintValueToString(fn));
}

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

    fn_struct.fn_handle = fn;

    return fn_struct;
}

static inline void llvm_verify_function(LLVMValueRef fn, const char* type)
{
    usize str_len = 0;
    const char* fn_name = LLVMGetValueName2(fn, &str_len);
    //print("\n\n%s\n\n", LLVMPrintValueToString(fn));
    if (LLVMVerifyFunction(fn, LLVMPrintMessageAction))
    {
        print("\n\n%s\n\n", LLVMPrintValueToString(fn));
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

static inline LLVMValueRef llvm_gen_expression(RedLLVMContext* llvm, IRExpression* expression)
{
    IRExpressionType type = expression->type;
    switch (type)
    {
        case IR_EXPRESSION_TYPE_BIN_EXPR:
        {
            IRBinaryExpr* bin_expr = &expression->bin_expr;
            LLVMValueRef left = llvm_gen_expression(llvm, bin_expr->left);
            LLVMValueRef right = llvm_gen_expression(llvm, bin_expr->right);
            TokenID op = bin_expr->op;

            switch (op)
            {
                case TOKEN_ID_CMP_LESS:
                    return LLVMBuildICmp(llvm->builder, LLVMIntSLT, left, right, "slt");
                case TOKEN_ID_CMP_GREATER:
                    return LLVMBuildICmp(llvm->builder, LLVMIntSGT, left, right, "sgt");
                case TOKEN_ID_CMP_EQ:
                    return LLVMBuildICmp(llvm->builder, LLVMIntEQ, left, right, "eq");
                case TOKEN_ID_PLUS:
                    return LLVMBuildAdd(llvm->builder, left, right, "add");
                case TOKEN_ID_DASH:
                    return LLVMBuildSub(llvm->builder, left, right, "sub");
                case TOKEN_ID_STAR:
                    return LLVMBuildMul(llvm->builder, left, right, "mul");
                case TOKEN_ID_SLASH:
                    // TODO: which division we want to perform
                    return LLVMBuildExactSDiv(llvm->builder, left, right, "exact_div");
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
                    u8 index = (u8)(param - llvm->current_fn->proto.params);
                    switch (sym_expr->use_type)
                    {
                        case LOAD:
                            return LLVMBuildLoad(llvm->builder, llvm->llvm_current_fn->param_alloc_arr[index], sb_ptr(param->name));
                        case STORE:
                            //return LLVMBuildStore(llvm->builder, llvm->llvm_current_fn->param_arr[index], llvm->llvm_current_fn->param_alloc_arr[index]);
                            RED_NOT_IMPLEMENTED;
                            break;
                        default:
                            RED_NOT_IMPLEMENTED;
                            break;
                    }
                }
                case IR_SYM_EXPR_TYPE_SYM:
                {
                    IRSymDeclStatement* sym = sym_expr->sym_decl;
                    IRSymDeclStatement* base_ptr = llvm->current_fn->sym_declarations.ptr;
                    u32 index = sym - base_ptr;

                    switch (sym_expr->use_type)
                    {
                        case LOAD:
                            return LLVMBuildLoad(llvm->builder, llvm->llvm_current_fn->alloca_buffer.ptr[index], sb_ptr(sym->name));
                        default:
                            RED_NOT_IMPLEMENTED;
                            break;
                    }
                    return null;
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
            return LLVMConstInt(LLVMInt32TypeInContext(llvm->context), n, int_lit->bigint.is_negative);
        }
        case IR_EXPRESSION_TYPE_VOID:
            return null;
        default:
            RED_NOT_IMPLEMENTED;
            break;
    }
    return null;
}

static inline LLVMValueRef llvm_gen_statement(RedLLVMContext* llvm, IRStatement* st);

static inline void llvm_gen_compound_statement(RedLLVMContext* llvm, IRCompoundStatement* compound_st)
{
    IRStatementBuffer* st_bf = &compound_st->stmts;
    IRStatement* st_it = st_bf->ptr;
    u32 st_count = st_bf->len;
    if (st_count > 0)
    {
        for (u32 i = 0; i < st_count; i++)
        {
            IRStatement* st = &st_it[i];
            llvm_gen_statement(llvm, st);
        }
    }
    else
    {
        RED_UNREACHABLE;
    }
}

static inline LLVMValueRef llvm_gen_statement(RedLLVMContext* llvm, IRStatement* st)
{
    IRStatementType type = st->type;
    switch (type)
    {
        case IR_ST_TYPE_RETURN_ST:
        {
            IRReturnStatement* ret_st = &st->return_st;
            IRExpressionType expr_type = ret_st->expression.type;
            IRExpression* expr = &ret_st->expression;
            LLVMValueRef ret;
            if (llvm->llvm_current_fn->ret_type != LLVMVoidTypeInContext(llvm->context))
            {
                LLVMValueRef ret_value = llvm_gen_expression(llvm, expr);
                redassert(ret_value);
                ret = LLVMBuildRet(llvm->builder, ret_value);
            }
            else
            {
                ret = LLVMBuildRetVoid(llvm->builder);
            }
            llvm->llvm_current_fn->return_already_emitted = true;
            return ret;
        }
        case IR_ST_TYPE_BRANCH_ST:
        {
            IRBranchStatement* branch_st = &st->branch_st;

            LLVMValueRef condition_value = llvm_gen_expression(llvm, &branch_st->condition);
            redassert(condition_value);

            LLVMBasicBlockRef llvm_if_bb = LLVMAppendBasicBlockInContext(llvm->context, llvm->llvm_current_fn->fn_handle, "if");
            redassert(llvm_if_bb);
            LLVMBasicBlockRef llvm_else_bb = null;
            bool else_statement = st->branch_st.else_block.stmts.len > 0;
            LLVMBasicBlockRef llvm_if_end_bb = null;
            if (else_statement)
            {
                llvm_else_bb = LLVMAppendBasicBlockInContext(llvm->context, llvm->llvm_current_fn->fn_handle, "else");
                redassert(llvm_else_bb);
            }
            else
            {
                llvm_if_end_bb = LLVMAppendBasicBlockInContext(llvm->context, llvm->llvm_current_fn->fn_handle, "if-end");
            }

            LLVMValueRef cond_br = LLVMBuildCondBr(llvm->builder, condition_value, llvm_if_bb, else_statement ? llvm_else_bb : llvm_if_end_bb);
            redassert(cond_br);
            LLVMPositionBuilderAtEnd(llvm->builder, llvm_if_bb);
            llvm->llvm_current_fn->depth_level++;

            llvm->llvm_current_fn->return_already_emitted = false;

            llvm_gen_compound_statement(llvm, &branch_st->if_block);

            bool return_emitted_in_all_branches_if = llvm->llvm_current_fn->return_already_emitted && else_statement;
            if (llvm->llvm_current_fn->return_already_emitted)
            {
                llvm->llvm_current_fn->return_already_emitted = false;
            }
            else
            {
                if (!llvm_if_end_bb)
                {
                    llvm_if_end_bb = LLVMAppendBasicBlockInContext(llvm->context, llvm->llvm_current_fn->fn_handle, "if-end");
                }
                redassert(llvm_if_end_bb);
                LLVMValueRef end_if_jmp = LLVMBuildBr(llvm->builder, llvm_if_end_bb);
                redassert(end_if_jmp);
            }

            bool return_emitted_in_all_branches_else;
            if (else_statement)
            {
                LLVMPositionBuilderAtEnd(llvm->builder, llvm_else_bb);

                llvm_gen_compound_statement(llvm, &branch_st->else_block);

                return_emitted_in_all_branches_else = llvm->llvm_current_fn->return_already_emitted;
                if (llvm->llvm_current_fn->return_already_emitted)
                {
                    llvm->llvm_current_fn->return_already_emitted = false;
                }
                else
                {
                    if (!llvm_if_end_bb)
                    {
                        llvm_if_end_bb = LLVMAppendBasicBlockInContext(llvm->context, llvm->llvm_current_fn->fn_handle, "if-end");
                    }
                    redassert(llvm_if_end_bb);
                    LLVMValueRef end_else_jmp = LLVMBuildBr(llvm->builder, llvm_if_end_bb);
                    redassert(end_else_jmp);
                    LLVMPositionBuilderAtEnd(llvm->builder, llvm_if_end_bb);
                }
            }
            else
            {
                LLVMPositionBuilderAtEnd(llvm->builder, llvm_if_end_bb);
                return_emitted_in_all_branches_else = false;
            }

            llvm->llvm_current_fn->return_already_emitted = return_emitted_in_all_branches_if && return_emitted_in_all_branches_else;
            llvm->llvm_current_fn->depth_level--;

            return null;
        }
        case IR_ST_TYPE_SYM_DECL_ST:
        {
            IRSymDeclStatement* decl_st = &st->sym_decl_st;
            LLVMTypeRef llvm_type = llvm_gen_type(llvm->context, &decl_st->type);
            LLVMValueRef alloca = LLVMBuildAlloca(llvm->builder, llvm_type, sb_ptr(decl_st->name));
            alloca_append(&llvm->llvm_current_fn->alloca_buffer, alloca);
            LLVMValueRef value_expression = llvm_gen_expression(llvm, &decl_st->value);
            if (value_expression)
            {
                LLVMBuildStore(llvm->builder, value_expression, alloca);
            }
            return null;
        }
        case IR_ST_TYPE_ASSIGN_ST:
        {
            IRSymAssignStatement* assign_st = &st->sym_assign_st;
            //LLVMValueRefBuffer* alloca_bf = &llvm->llvm_current_fn->alloca_buffer;
            //u32 alloca_count = alloca_bf->len;
            //LLVMValueRef* alloca_it = alloca_bf->ptr;
            //for (u32 i = 0; i < alloca_count; i++)
            //{
            //    LLVMValueRef alloca = alloca_it[i];
            //    if (alloca =)
            //}
            IRSymExpr* sym_expr = &assign_st->left;
            IRSymExprType sym_expr_type = sym_expr->type;
            LLVMValueRef alloca = null;
            switch (sym_expr_type)
            {
                case IR_SYM_EXPR_TYPE_SYM:
                {
                    IRSymDeclStatement* sym_decl = sym_expr->sym_decl;
                    IRSymDeclStatement* sym_decl_base_ptr = llvm->current_fn->sym_declarations.ptr;
                    u32 index = sym_decl - sym_decl_base_ptr;
                    alloca = *(llvm->llvm_current_fn->alloca_buffer.ptr + index);
                    break;
                }
                default:
                    RED_NOT_IMPLEMENTED;
                    break;
            }

            IRExpression* right_value = assign_st->right;
            LLVMValueRef value = llvm_gen_expression(llvm, right_value);
            redassert(value);

            LLVMBuildStore(llvm->builder, value, alloca);

            return null;
        }
        default:
            RED_NOT_IMPLEMENTED;
            return null;
    }
}

static inline void llvm_gen_fn_definition(RedLLVMContext* llvm)
{
    /* Prototype */
    RedLLVMFn fn_struct = llvm_gen_fn_proto(llvm->context, llvm->module, &llvm->current_fn->proto);
    llvm->llvm_current_fn = &fn_struct;
    LLVMValueRef fn = fn_struct.fn_handle;

    /* Definition */
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(llvm->context, fn, "entry");
    llvm->builder = LLVMCreateBuilderInContext(llvm->context);
    LLVMPositionBuilderAtEnd(llvm->builder, entry);

    u8 param_count = llvm->current_fn->proto.param_count;
    LLVMValueRef* params = null;
    IRParamDecl* ir_params = llvm->current_fn->proto.params;

    if (param_count > 0)
    {
        redassert(param_count < max_param_count);
        redassert(ir_params);
        params = fn_struct.param_arr;
        LLVMGetParams(fn, params);

        for (usize i = 0; i < param_count; i++)
        {
            LLVMSetValueName(params[i], ir_params[i].name->ptr);
            fn_struct.param_alloc_arr[i] = LLVMBuildAlloca(llvm->builder, LLVMTypeOf(params[i]), "");
            LLVMBuildStore(llvm->builder, params[i], fn_struct.param_alloc_arr[i]);
        }
    }

    u32 st_count = llvm->current_fn->body.stmts.len;
    if (st_count > 0)
    {
        IRCompoundStatement* body = &llvm->current_fn->body;
        llvm_gen_compound_statement(llvm, body);
    }
    else if (st_count == 0)
    {
        LLVMValueRef ret_value = LLVMConstInt(LLVMInt32TypeInContext(llvm->context), 0, false);
        LLVMBuildRet(llvm->builder, ret_value);
    }
    else
    {
        RED_UNREACHABLE;
    }

#if RED_LLVM_VERBOSE
    llvm_verify_function(fn, "definition");
#endif
}

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
        ctx.current_fn = fn_def_it;
        llvm_gen_fn_definition(&ctx);
    }

    llvm_verify_module(ctx.module);
}
