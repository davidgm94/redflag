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
#include <llvm-c/Comdat.h>
#include <llvm-c/Transforms/PassManagerBuilder.h>
#include <llvm-c/Transforms/AggressiveInstCombine.h>
#include <llvm-c/Transforms/Coroutines.h>
#include <llvm-c/Transforms/InstCombine.h>
#include <llvm-c/Transforms/IPO.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Transforms/Utils.h>
#include <llvm-c/Transforms/Vectorize.h>

#include "microsoft_craziness.h"
#include "lld.h"

#include <stdio.h>

typedef struct TypeDeclarationLLVM TypeDeclarationLLVM;

static LLVMTypeRef llvm_primitive_types[IR_TYPE_PRIMITIVE_COUNT];

#define DW_ATE_address 0x1
#define DW_ATE_boolean 0x2
#define DW_ATE_complex_float 0x3
#define DW_ATE_float 0x4
#define DW_ATE_signed 0x5
#define DW_ATE_signed_char 0x6
#define DW_ATE_unsigned 0x7
#define DW_ATE_unsigned_char 0x8

static const LLVMDWARFTypeEncoding dwarf_encodings[IR_TYPE_PRIMITIVE_COUNT] =
{
    DW_ATE_unsigned_char,
    DW_ATE_unsigned,
    DW_ATE_unsigned,
    DW_ATE_unsigned,
    DW_ATE_signed_char,
    DW_ATE_signed,
    DW_ATE_signed,
    DW_ATE_signed,
    DW_ATE_float,
    DW_ATE_float,
    DW_ATE_complex_float, //buggy
    DW_ATE_boolean,
};

#define MAX_PARAM_COUNT 256

typedef struct LocalStringLLVM
{
    IRSymDeclStatement* decl_ptr;
    LLVMValueRef value;
} LocalStringLLVM;

typedef struct FnProtoLLVM
{
    struct
    {
        LLVMMetadataRef param_types[MAX_PARAM_COUNT];
    } debug;
    LLVMTypeRef param_types[MAX_PARAM_COUNT];
    LLVMTypeRef return_type;
    LLVMTypeRef fn_type;
    LLVMValueRef handle;
    u8 param_count;
} FnProtoLLVM;

GEN_BUFFER_STRUCT(LLVMValueRef)
GEN_BUFFER_FUNCTIONS(llvm_value, vb, LLVMValueRefBuffer, LLVMValueRef)
GEN_BUFFER_STRUCT(LocalStringLLVM)
GEN_BUFFER_FUNCTIONS(local_str, lsb, LocalStringLLVMBuffer, LocalStringLLVM)
GEN_BUFFER_STRUCT(FnProtoLLVM)
GEN_BUFFER_FUNCTIONS(llvm_fn_proto, llvm_fpb, FnProtoLLVMBuffer, FnProtoLLVM)
GEN_BUFFER_STRUCT(LLVMTypeRef)
GEN_BUFFER_FUNCTIONS(llvm_type, ltb, LLVMTypeRefBuffer, LLVMTypeRef)

typedef struct TypeDeclarationLLVM
{
    LLVMTypeRef type;
    LLVMTypeRefBuffer child_types;
} TypeDeclarationLLVM;
GEN_BUFFER_STRUCT(TypeDeclarationLLVM)
GEN_BUFFER_FUNCTIONS(llvm_type_decl, ltb, TypeDeclarationLLVMBuffer, TypeDeclarationLLVM)

typedef struct CurrentFnLLVM
{
    LLVMValueRef param_array[MAX_PARAM_COUNT];
    LLVMValueRef param_alloca_array[MAX_PARAM_COUNT];
    LLVMValueRefBuffer alloca_buffer;
    LocalStringLLVMBuffer local_string_buffer;
    FnProtoLLVM* proto;
    bool return_already_emitted;
} CurrentFnLLVM;

typedef struct ModuleContext
{
    LLVMModuleRef handle;
    LLVMBuilderRef builder;

    struct
    {
        LLVMDIBuilderRef builder;
        LLVMMetadataRef file;
    } debug;
    
    CurrentFnLLVM* current_fn;

    TypeDeclarationLLVMBuffer type_declarations;
    LLVMValueRefBuffer global_sym_buffer;
    FnProtoLLVMBuffer fn_proto_buffer;
} ModuleContext;

typedef struct TargetLLVM
{
    LLVMTargetRef handle;
    char* triple;
    LLVMTargetMachineRef machine;
    LLVMTargetDataRef data;
} TargetLLVM;

static inline void llvm_verify_function(LLVMValueRef fn, const char* type, bool silent);
static inline void llvm_debug_fn(LLVMValueRef fn)
{
    print("Debugging function\n\n%s\n\n", LLVMPrintValueToString(fn));
}

static inline LLVMValueRef llvm_gen_expression(LLVMContextRef context, ModuleContext* module, IRModule* ir_module, IRFunctionDefinition* current_fn, IRExpression* expression, IRType* expected_type);

static inline LocalStringLLVM* find_local_string(LocalStringLLVMBuffer* local_str_bf, IRSymDeclStatement* sym)
{
    u32 local_string_count = local_str_bf->len;
    LocalStringLLVM* ptr = local_str_bf->ptr;
    for (u32 i = 0; i < local_string_count; i++)
    {
        LocalStringLLVM* local_str = &ptr[i];
        if (sb_cmp(local_str->decl_ptr->name, sym->name))
        {
            return local_str;
        }
    }

    return null;
}

static inline LLVMTypeRef llvm_gen_type(LLVMContextRef context, ModuleContext* module, IRModule* ir_module, IRType* type)
{
    if (type)
    {
        TypeKind kind = type->kind;
        switch (kind)
        {
            case TYPE_KIND_PRIMITIVE:
            {
                IRTypePrimitive primitive_kind = type->primitive_type;
                redassert(primitive_kind < IR_TYPE_PRIMITIVE_COUNT);
                return llvm_primitive_types[primitive_kind];
            }
            case TYPE_KIND_ARRAY:
            {
                LLVMTypeRef base_type = llvm_gen_type(context, module, ir_module, type->array_type.base_type);
                IRExpression* elem_count = type->array_type.elem_count_expr;
                IRExpressionType type = elem_count->type;
                u64 arr_elem_count;
                switch (type)
                {
                    case IR_EXPRESSION_TYPE_INT_LIT:
                        redassert(elem_count->int_literal.bigint.digit_count == 1);
                        redassert(elem_count->int_literal.bigint.is_negative == false);
                        arr_elem_count = elem_count->int_literal.bigint.digit;
                        break;
                    default:
                        RED_NOT_IMPLEMENTED;
                        return null;
                }
                LLVMTypeRef array_type = LLVMArrayType(base_type, arr_elem_count);
                return array_type;
            }
            case TYPE_KIND_STRUCT:
            {
                if (type->struct_type)
                {
                    u32 struct_count = ir_module->struct_decls.len;
                    if (struct_count > 0)
                    {
                        IRStructDecl* struct_decl_ptr = ir_module->struct_decls.ptr;
                        IRStructDecl* struct_decl = type->struct_type;
                        u32 index = struct_decl - struct_decl_ptr;
                        return module->type_declarations.ptr[index].type;
                    }
                }
                RED_UNREACHABLE;
                return null;
            }
            case TYPE_KIND_ENUM:
            {
                LLVMTypeRef enum_type = llvm_gen_type(context, module, ir_module, &type->enum_type->type);
                return enum_type;
            }
            case TYPE_KIND_POINTER:
            {
                LLVMTypeRef pointer_type = LLVMPointerType(llvm_gen_type(context, module, ir_module, type->pointer_type.base_type), 0);
                return pointer_type;
            }
            case TYPE_KIND_VOID:
            {
                LLVMTypeRef void_type = LLVMVoidTypeInContext(context);
                return void_type;
            }
            case TYPE_KIND_RAW_STRING:
            {
                LLVMTypeRef string_type = LLVMPointerType(llvm_primitive_types[IR_TYPE_PRIMITIVE_U8], 0);
                return string_type;
            }
            default:
                RED_NOT_IMPLEMENTED;
                return null;
        }
    }

    return LLVMVoidTypeInContext(context);
}

static inline void llvm_verify_function(LLVMValueRef fn, const char* type, bool silent)
{
    usize str_len = 0;
    //print("\n\n%s\n\n", LLVMPrintValueToString(fn));
    if (LLVMVerifyFunction(fn, LLVMPrintMessageAction))
    {
        const char* fn_name = LLVMGetValueName2(fn, &str_len);
        print("\n\n%s\n\n", LLVMPrintValueToString(fn));
        os_exit_with_message("%s FAIL\n", fn_name);
    }
    else
    {
        if (!silent)
        {
            const char* fn_name = LLVMGetValueName2(fn, &str_len);
            print("%s OK\n", fn_name);
        }
    }
}

static inline bool llvm_verify_module(LLVMModuleRef module)
{
#if 1
#if RED_LLVM_VERBOSE
    print("\n\n****\n");
    print("%s\n****\n\n", LLVMPrintModuleToString(module));
#endif
#endif
    char* error = NULL;
#if RED_LLVM_VERBOSE
    LLVMBool errors = LLVMVerifyModule(module, LLVMPrintMessageAction, &error);
    if (errors)
#else
    if (LLVMVerifyModule(module, LLVMReturnStatusAction, &error))
#endif
    {
#if RED_LLVM_VERBOSE
        os_exit_with_message("\nFailed to verify module: %s\n\n", error);
#endif
    }
    else
    {
#if RED_LLVM_VERBOSE
        prints("\nVerified module\n");
#endif
    }

    return !errors;
}

// add extra checks
// This works both for fn call expression and fn call statement
static inline LLVMValueRef llvm_gen_fn_call(LLVMContextRef context, ModuleContext* module, IRModule* ir_module, IRFunctionDefinition* current_fn, IRFunctionCallExpr* fn_call)
{
    redassert(sizeof(IRFunctionCallExpr) == sizeof(IRFunctionCallStatement));
    IRModule* ref_module = fn_call->fn->module;
    if (ref_module != ir_module)
    {
        RED_NOT_IMPLEMENTED;
    }
    LLVMValueRef fn = LLVMGetNamedFunction(module->handle, sb_ptr(fn_call->fn->name)); // <- @this is bullshit

    LLVMValueRef arg_values[256];
    LLVMValueRef* arg_ptr = fn_call->arg_count > 0 ? arg_values : NULL;
    for (u32 i = 0; i < fn_call->arg_count; i++)
    {
        redassert(i < 256);
        IRExpression* arg_expr = &fn_call->args[i];
        arg_values[i] = llvm_gen_expression(context, module, ir_module, current_fn, arg_expr, NULL);
    }
    LLVMValueRef fn_call_value = LLVMBuildCall(module->builder, fn, arg_ptr, fn_call->arg_count, sb_ptr(fn_call->fn->name));
    return fn_call_value;
}

static inline LLVMValueRef llvm_gen_expression(LLVMContextRef context, ModuleContext* module, IRModule* ir_module, IRFunctionDefinition* current_fn, IRExpression* expression, IRType* expected_type)
{
    IRExpressionType type = expression->type;
    switch (type)
    {
        case IR_EXPRESSION_TYPE_BIN_EXPR:
        {
            IRBinaryExpr* bin_expr = &expression->bin_expr;
            LLVMValueRef left = llvm_gen_expression(context, module, ir_module, current_fn, bin_expr->left, NULL);
            LLVMValueRef right = llvm_gen_expression(context, module, ir_module, current_fn, bin_expr->right, NULL);
            TokenID op = bin_expr->op;

            switch (op)
            {
                case TOKEN_ID_CMP_LESS:
                    return LLVMBuildICmp(module->builder, LLVMIntSLT, left, right, "slt");
                case TOKEN_ID_CMP_GREATER:
                    return LLVMBuildICmp(module->builder, LLVMIntSGT, left, right, "sgt");
                case TOKEN_ID_CMP_EQ:
                    return LLVMBuildICmp(module->builder, LLVMIntEQ, left, right, "eq");
                case TOKEN_ID_PLUS:
                    return LLVMBuildAdd(module->builder, left, right, "add");
                case TOKEN_ID_DASH:
                    return LLVMBuildSub(module->builder, left, right, "sub");
                case TOKEN_ID_STAR:
                    return LLVMBuildMul(module->builder, left, right, "mul");
                case TOKEN_ID_SLASH:
                    // TODO: which division we want to perform
                    return LLVMBuildExactSDiv(module->builder, left, right, "exact_div");
                default:
                    RED_NOT_IMPLEMENTED;
                    break;
            }
            break;
        }
        case IR_EXPRESSION_TYPE_SUBSCRIPT_ACCESS:
        {
            IRSubscriptAccess* subscript_access = &expression->subscript_access;
            IRSymbolSubscriptType subs_type = subscript_access->subscript_type;
            TypeKind data_type = subscript_access->parent.type;
            switch (subs_type)
            {
                case AST_SYMBOL_SUBSCRIPT_TYPE_FIELD_ACCESS:
                {
                    switch (data_type)
                    {
                        case TYPE_KIND_STRUCT:
                        {
                            IRFieldDecl* field_ptr = subscript_access->parent.struct_p->fields;
                            u32 field_count = subscript_access->parent.struct_p->field_count;

                            if (field_count > 0)
                            {
                                for (u32 i = 0; i < field_count; i++)
                                {
                                    IRFieldDecl* field = &field_ptr[i];
                                    if (sb_cmp(field->name, subscript_access->name))
                                    {
                                        return LLVMConstInt(LLVMInt32TypeInContext(context), i, false);
                                    }
                                }
                            }
                            RED_NOT_IMPLEMENTED;
                            break;
                        }
                        default:
                            RED_NOT_IMPLEMENTED;
                            break;
                    }
                }
                default:
                    RED_NOT_IMPLEMENTED;
                    break;
            }
            if (subscript_access->subscript)
            {
                llvm_gen_expression(context, module, ir_module, current_fn, subscript_access->subscript, NULL);
            }
            return null;
        }
        case IR_EXPRESSION_TYPE_SYM_EXPR:
        {
            IRSymExpr* sym_expr = &expression->sym_expr;
            IRSymExprType se_type = sym_expr->type;
            IRExpression* subscript = sym_expr->subscript;
            if (subscript)
            {
                IRSymbolSubscriptType subscript_type = sym_expr->subscript->subscript_access.subscript_type;
                switch (subscript_type)
                {
                    case AST_SYMBOL_SUBSCRIPT_TYPE_ARRAY_ACCESS:
                    {
                        //RedType base_type = ast_to_ir_find_expression_type(expression);
                        switch (se_type)
                        {
                            case IR_SYM_EXPR_TYPE_PARAM:
                                RED_NOT_IMPLEMENTED;
                                return null;
                            case IR_SYM_EXPR_TYPE_SYM:
                            {
                                IRSymDeclStatement* sym = sym_expr->sym_decl;
                                IRSymDeclStatement* base_ptr = current_fn->sym_declarations.ptr;
                                u32 index = sym - base_ptr;
                                LLVMValueRef arr_alloca = module->current_fn->alloca_buffer.ptr[index];
                                LLVMValueRef zero = LLVMConstInt(LLVMIntTypeInContext(context, 32), 0, true);
                                LLVMValueRef index_value = llvm_gen_expression(context, module, ir_module, current_fn, sym_expr->subscript, NULL);
                                LLVMValueRef indices[2] =
                                {
                                    zero,
                                    index_value,
                                };
                                //LLVMTypeRef llvm_base_type = llvm_gen_type(llvm, &base_type);
                                ;
                                //array_subscript_value = LLVMBuildInBoundsGEP2(llvm->builder, llvm_base_type, arr_alloca, indices, array_length(indices), "arr_subscript_access");
                                LLVMValueRef array_subscript_value = LLVMBuildInBoundsGEP(module->builder, arr_alloca, indices, array_length(indices), "arridxaccess");
                                switch (sym_expr->use_type)
                                {
                                    case LOAD:
                                        return LLVMBuildLoad(module->builder, array_subscript_value, "arridxload");
                                    case STORE:
                                        // The store takes place in the assign statement
                                        // TODO: this probably is buggy
                                        return array_subscript_value;
                                    default:
                                        RED_NOT_IMPLEMENTED;
                                        return null;
                                }
                            }
                            default:
                                RED_NOT_IMPLEMENTED;
                                return null;
                        }
                    }
                    case AST_SYMBOL_SUBSCRIPT_TYPE_FIELD_ACCESS:
                        switch (se_type)
                        {
                            case IR_SYM_EXPR_TYPE_SYM:
                            {
                                IRSymDeclStatement* sym = sym_expr->sym_decl;
                                IRSymDeclStatement* base_ptr = current_fn->sym_declarations.ptr;
                                u32 index = sym - base_ptr;
                                LLVMValueRef alloca = module->current_fn->alloca_buffer.ptr[index];
                                LLVMValueRef zero = LLVMConstInt(LLVMIntTypeInContext(context, 32), 0, true);
                                LLVMValueRef index_value = llvm_gen_expression(context, module, ir_module, current_fn, sym_expr->subscript, NULL);
                                LLVMValueRef indices[2] =
                                {
                                    zero,
                                    index_value,
                                };

                                return LLVMBuildInBoundsGEP(module->builder, alloca, indices, array_length(indices), "struct_field_access");
                            }
                            case IR_SYM_EXPR_TYPE_ENUM:
                            {
                                IREnumDecl* enum_decl = sym_expr->enum_decl;
                                redassert(enum_decl->type.kind == TYPE_KIND_PRIMITIVE);
                                IRTypePrimitive primitive_type = enum_decl->type.primitive_type;
                                // TODO: we should put this before LLVM Codegen
                                // TODO: even better: for enums, don't store names but the value
                                SB* field_name = sym_expr->subscript->subscript_access.name;

                                u32 field_count = enum_decl->fields.len;
                                IREnumField* field_ptr = enum_decl->fields.ptr;
                                for (u32 i = 0; i < field_count; i++)
                                {
                                    IREnumField* field = &field_ptr[i];
                                    if (sb_cmp(field->name, field_name))
                                    {
                                        switch (primitive_type)
                                        {
                                            case IR_TYPE_PRIMITIVE_U32:
                                                return LLVMConstInt(llvm_primitive_types[IR_TYPE_PRIMITIVE_U32], field->value.unsigned64, false);
                                            default:
                                                RED_NOT_IMPLEMENTED;
                                                break;
                                        }
                                    }
                                }
                            }
                            default:
                                RED_NOT_IMPLEMENTED;
                                break;
                        }
                        RED_NOT_IMPLEMENTED;
                        return null;
                    default:
                        RED_NOT_IMPLEMENTED;
                        return null;
                }
                RED_NOT_IMPLEMENTED;
                return null;
            }
            else
            {
                switch (se_type)
                {
                    case IR_SYM_EXPR_TYPE_PARAM:
                    {
                        IRParamDecl* param = sym_expr->param_decl;
                        u8 index = (u8)(param - current_fn->proto->params);
                        switch (sym_expr->use_type)
                        {
                            case LOAD:
                                return LLVMBuildLoad(module->builder, module->current_fn->param_alloca_array[index], sb_ptr(param->name));
                            case STORE:
                                //return LLVMBuildStore(module->builder, llvm->llvm_current_fn->param_arr[index], llvm->llvm_current_fn->param_alloca_array[index]);
                                return module->current_fn->param_alloca_array[index];
                                break;
                            default:
                                RED_NOT_IMPLEMENTED;
                                break;
                        }
                    }
                    case IR_SYM_EXPR_TYPE_SYM:
                    {
                        IRSymDeclStatement* sym = sym_expr->sym_decl;
                        if (sym->type.kind == TYPE_KIND_RAW_STRING)
                        {
                            LocalStringLLVM* local_string = find_local_string(&module->current_fn->local_string_buffer, sym);
                            redassert(local_string);
                            return local_string->value;
                        }
                        else
                        {
                            IRSymDeclStatement* base_ptr = current_fn->sym_declarations.ptr;
                            u64 index = sym - base_ptr;

                            switch (sym_expr->use_type)
                            {
                                case LOAD:
                                    return LLVMBuildLoad(module->builder, module->current_fn->alloca_buffer.ptr[index], sb_ptr(sym->name));
                                case STORE:
                                    return module->current_fn->alloca_buffer.ptr[index];
                                default:
                                    RED_NOT_IMPLEMENTED;
                                    return null;
                            }
                        }
                    }
                    case IR_SYM_EXPR_TYPE_GLOBAL_SYM:
                    {
                        IRSymDeclStatement* global_sym = sym_expr->global_sym_decl;
                        IRSymDeclStatement* base_ptr = ir_module->global_sym_decls.ptr;
                        u64 index = global_sym - base_ptr;

                        switch (sym_expr->use_type)
                        {
                            case LOAD:
                                return LLVMBuildLoad(module->builder, module->global_sym_buffer.ptr[index], sb_ptr(global_sym->name));
                                /* TODO: probably buggy */
                            case STORE:
                                return module->global_sym_buffer.ptr[index];
                            default:
                                RED_NOT_IMPLEMENTED;
                                return null;
                        }
                    }
                    default:
                        RED_NOT_IMPLEMENTED;
                        break;
                }
            }
            break;
        }
        case IR_EXPRESSION_TYPE_INT_LIT:
        {
            IRIntLiteral* int_lit = &expression->int_literal;
            redassert(int_lit->bigint.digit_count == 1);
            u64 n = int_lit->bigint.digit;
            // TODO: fix type
            redassert(int_lit->type < IR_TYPE_PRIMITIVE_COUNT);
            return LLVMConstInt(llvm_primitive_types[int_lit->type], n, int_lit->bigint.is_negative);
        }
        case IR_EXPRESSION_TYPE_ARRAY_LIT:
        {
            IRArrayLiteral* array_lit = &expression->array_literal;
            u64 lit_count = array_lit->expression_count;
            LLVMValueRef* lit_arr = NEW(LLVMValueRef, lit_count);
            for (s32 i = 0; i < lit_count; i++)
            {
                lit_arr[i] = llvm_gen_expression(context, module, ir_module, current_fn, &array_lit->expressions[i], NULL);
            }
            LLVMTypeRef lit_type = LLVMTypeOf(lit_arr[0]);
            LLVMValueRef llvm_array_lit = LLVMConstArray(lit_type, lit_arr, lit_count);
            return llvm_array_lit;
        }
        case IR_EXPRESSION_TYPE_STRING_LIT:
        {
            IRStringLiteral* string_lit = &expression->string_literal;
            LLVMValueRef string_lit_llvm = LLVMBuildGlobalStringPtr(module->builder, sb_ptr(string_lit->str_lit), "string_lit");
            return string_lit_llvm;
        }
        case IR_EXPRESSION_TYPE_FN_CALL_EXPR:
        {
            IRFunctionCallExpr* fn_call = &expression->fn_call_expr;
            LLVMValueRef fn_call_llvm = llvm_gen_fn_call(context, module, ir_module, current_fn, fn_call);
            return fn_call_llvm;
        }
        case IR_EXPRESSION_TYPE_VOID:
            return null;
        default:
            RED_NOT_IMPLEMENTED;
            break;
    }
    return null;
}

static inline LLVMValueRef llvm_gen_statement(LLVMContextRef context, ModuleContext* module, IRModule* ir_module, IRFunctionDefinition* current_fn, IRStatement* st);

static inline void llvm_gen_compound_statement(LLVMContextRef context, ModuleContext* module, IRModule* ir_module, IRFunctionDefinition* current_fn, IRCompoundStatement* compound_st)
{
    IRStatementBuffer* st_bf = &compound_st->stmts;
    IRStatement* st_it = st_bf->ptr;
    u32 st_count = st_bf->len;
    if (st_count > 0)
    {
        for (u32 i = 0; i < st_count; i++)
        {
            IRStatement* st = &st_it[i];
            llvm_gen_statement(context, module, ir_module, current_fn, st);
        }
    }
    else
    {
        RED_UNREACHABLE;
    }
}

typedef struct LLVMSwitchCases
{
    LLVMBasicBlockRef bb;
    LLVMValueRef block;
} LLVMSwitchCases;

static inline LLVMValueRef llvm_gen_statement(LLVMContextRef context, ModuleContext* module, IRModule* ir_module, IRFunctionDefinition* current_fn, IRStatement* st)
{
    IRStatementType type = st->type;
    switch (type)
    {
        case IR_ST_TYPE_RETURN_ST:
        {
            IRReturnStatement* ret_st = &st->return_st;
            IRExpression* expr = &ret_st->expression;
            LLVMValueRef ret;
            if (module->current_fn->proto->return_type != LLVMVoidTypeInContext(context))
            {
                LLVMValueRef ret_value = llvm_gen_expression(context, module, ir_module, current_fn, expr, NULL);
                redassert(ret_value);
                LLVMTypeRef expr_type = LLVMTypeOf(ret_value);
                bool type_mismatch = expr_type != module->current_fn->proto->return_type;
                if (type_mismatch)
                {
                    ret_value = LLVMBuildBitCast(module->builder, ret_value, module->current_fn->proto->return_type, "cast");
                }
                ret = LLVMBuildRet(module->builder, ret_value);
            }
            else
            {
                ret = LLVMBuildRetVoid(module->builder);
            }
            module->current_fn->return_already_emitted = true;
            return ret;
        }
        case IR_ST_TYPE_BRANCH_ST:
        {
            IRBranchStatement* branch_st = &st->branch_st;

            LLVMValueRef condition_value = llvm_gen_expression(context, module, ir_module, current_fn, &branch_st->condition, NULL);
            redassert(condition_value);

            LLVMBasicBlockRef llvm_if_bb = LLVMAppendBasicBlockInContext(context, module->current_fn->proto->handle, "if");
            redassert(llvm_if_bb);
            LLVMBasicBlockRef llvm_else_bb = null;
            bool else_statement = st->branch_st.else_block;
            LLVMBasicBlockRef llvm_if_end_bb = null;
            if (else_statement)
            {
                llvm_else_bb = LLVMAppendBasicBlockInContext(context, module->current_fn->proto->handle, "else");
                redassert(llvm_else_bb);
            }
            else
            {
                llvm_if_end_bb = LLVMAppendBasicBlockInContext(context, module->current_fn->proto->handle, "if-end");
            }

            LLVMValueRef cond_br = LLVMBuildCondBr(module->builder, condition_value, llvm_if_bb, else_statement ? llvm_else_bb : llvm_if_end_bb);
            redassert(cond_br);
            LLVMPositionBuilderAtEnd(module->builder, llvm_if_bb);

            module->current_fn->return_already_emitted = false;

            llvm_gen_compound_statement(context, module, ir_module, current_fn, &branch_st->if_block);

            bool return_emitted_in_all_branches_if = module->current_fn->return_already_emitted && else_statement;
            if (module->current_fn->return_already_emitted)
            {
                module->current_fn->return_already_emitted = false;
            }
            else
            {
                if (!llvm_if_end_bb)
                {
                    llvm_if_end_bb = LLVMAppendBasicBlockInContext(context, module->current_fn->proto->handle, "if-end");
                }
                redassert(llvm_if_end_bb);
                LLVMValueRef end_if_jmp = LLVMBuildBr(module->builder, llvm_if_end_bb);
                redassert(end_if_jmp);
            }

            bool return_emitted_in_all_branches_else = false;
            if (else_statement)
            {
                LLVMPositionBuilderAtEnd(module->builder, llvm_else_bb);

                llvm_gen_statement(context, module, ir_module, current_fn, branch_st->else_block);

                return_emitted_in_all_branches_else = module->current_fn->return_already_emitted;

                if (module->current_fn->return_already_emitted)
                {
                    module->current_fn->return_already_emitted = false;
                }
                else
                {
                    if (!llvm_if_end_bb)
                    {
                        llvm_if_end_bb = LLVMAppendBasicBlockInContext(context, module->current_fn->proto->handle, "if-end");
                    }
                    redassert(llvm_if_end_bb);
                    LLVMValueRef end_else_jmp = LLVMBuildBr(module->builder, llvm_if_end_bb);
                    redassert(end_else_jmp);
                }
            }
            if (llvm_if_end_bb)
            {
                LLVMPositionBuilderAtEnd(module->builder, llvm_if_end_bb);
            }

            module->current_fn->return_already_emitted = return_emitted_in_all_branches_if && return_emitted_in_all_branches_else;

            return null;
        }
        case IR_ST_TYPE_SWITCH_ST:
        {
            IRSwitchStatement* sw_st = &st->switch_st;
            IRSwitchCaseBuffer* swcb = &sw_st->cases;
            u32 sw_case_count = swcb->len;
            IRSwitchCase* sw_case_ptr = swcb->ptr;
            IRSwitchCase* sw_default = null;

            for (u32 i = 0; i < sw_case_count; i++)
            {
                IRSwitchCase* sw_case = &sw_case_ptr[i];
                // If default
                if (sw_case->case_expr.type == IR_EXPRESSION_TYPE_VOID)
                {
                    sw_default = sw_case;
                    break;
                }
            }

            LLVMValueRef sw_expr = llvm_gen_expression(context, module, ir_module, current_fn, &sw_st->switch_expr, NULL);

            //LLVMBasicBlockRef sw_def_bb = sw_default ? LLVMAppendBasicBlockInContext(llvm->context, llvm->llvm_current_fn.fn_handle, "default_sw_case") : null;
            //LLVMBasicBlockRef sw_end_bb = LLVMAppendBasicBlockInContext(llvm->context, llvm->llvm_current_fn.fn_handle, "sw_end");
            LLVMBasicBlockRef sw_def_bb = sw_default ? LLVMCreateBasicBlockInContext(context, "default_sw_case") : null;
            LLVMBasicBlockRef sw_end_bb = LLVMCreateBasicBlockInContext(context, "sw_end");
            LLVMValueRef llvm_switch = LLVMBuildSwitch(module->builder, sw_expr, sw_def_bb ? sw_def_bb : sw_end_bb, sw_case_count);

            bool return_emitted_in_all_branches = true;
            for (u32 i = 0; i < sw_case_count; i++)
            {
                IRSwitchCase* sw_case = &sw_case_ptr[i];
                // If not default
                if (sw_case->case_expr.type != IR_EXPRESSION_TYPE_VOID)
                {
                    module->current_fn->return_already_emitted = false;
                    LLVMBasicBlockRef case_bb = LLVMAppendBasicBlockInContext(context, module->current_fn->proto->handle, "sw_case");
                    LLVMPositionBuilderAtEnd(module->builder, case_bb);
                    llvm_gen_compound_statement(context, module, ir_module, current_fn, &sw_case->case_body);
                    if (!module->current_fn->return_already_emitted)
                    {
                        return_emitted_in_all_branches = false;
                        LLVMBuildBr(module->builder, sw_end_bb);
                    }
                    LLVMAddCase(llvm_switch, llvm_gen_expression(context, module, ir_module, current_fn, &sw_case->case_expr, NULL), case_bb);
                }
            }

            if (sw_def_bb)
            {
                LLVMAppendExistingBasicBlock(module->current_fn->proto->handle, sw_def_bb);
                module->current_fn->return_already_emitted = false;
                LLVMPositionBuilderAtEnd(module->builder, sw_def_bb);
                llvm_gen_compound_statement(context, module, ir_module, current_fn, &sw_default->case_body);
                if (!module->current_fn->return_already_emitted)
                {
                    return_emitted_in_all_branches = false;
                    LLVMBuildBr(module->builder, sw_end_bb);
                }
            }

            LLVMAppendExistingBasicBlock(module->current_fn->proto->handle, sw_end_bb);
            LLVMPositionBuilderAtEnd(module->builder, sw_end_bb);
            if (return_emitted_in_all_branches)
            {
                LLVMBuildUnreachable(module->builder);
            }

            return llvm_switch;
        }
        case IR_ST_TYPE_SYM_DECL_ST:
        {
            IRSymDeclStatement* decl_st = &st->sym_decl_st;
            if (decl_st->type.kind == TYPE_KIND_RAW_STRING)
            {
                LLVMValueRef str_ptr = LLVMBuildGlobalStringPtr(module->builder, sb_ptr(decl_st->value.string_literal.str_lit), sb_ptr(decl_st->name));
                local_str_append(&module->current_fn->local_string_buffer, (const LocalStringLLVM) { .decl_ptr = decl_st, .value = str_ptr });
                return str_ptr;
            }
            else
            {
                LLVMTypeRef llvm_type = llvm_gen_type(context, module, ir_module, &decl_st->type);
                LLVMValueRef alloca = LLVMBuildAlloca(module->builder, llvm_type, sb_ptr(decl_st->name));
                llvm_value_append(&module->current_fn->alloca_buffer, alloca);
                LLVMValueRef value_expression = llvm_gen_expression(context, module, ir_module, current_fn, &decl_st->value, &decl_st->type);
                if (value_expression)
                {
                    LLVMBuildStore(module->builder, value_expression, alloca);
                }
                return null;
            }
        }
        case IR_ST_TYPE_ASSIGN_ST:
        {
            IRSymAssignStatement* assign_st = &st->sym_assign_st;

            IRExpression* left_expr = assign_st->left;
            LLVMValueRef left_value = llvm_gen_expression(context, module, ir_module, current_fn, left_expr, NULL);
            IRExpressionType left_expr_type = left_expr->type;

            // If pointer type, emit a load
            switch (left_expr_type)
            {
                case IR_EXPRESSION_TYPE_SYM_EXPR:
                {
                    IRSymExprType left_expr_sym_expr_type = left_expr->sym_expr.type;
                    switch (left_expr_sym_expr_type)
                    {
                        case IR_SYM_EXPR_TYPE_PARAM:
                            if (left_expr->sym_expr.param_decl->type.kind == TYPE_KIND_POINTER)
                            {
                                left_value = LLVMBuildLoad(module->builder, left_value, "ptrload");
                            }
                            break;
                        case IR_SYM_EXPR_TYPE_SYM:
                            if (left_expr->sym_expr.sym_decl->type.kind == TYPE_KIND_POINTER)
                            {
                                left_value = LLVMBuildLoad(module->builder, left_value, "ptrload");
                            }
                            break;
                        case IR_SYM_EXPR_TYPE_GLOBAL_SYM:
                            if (left_expr->sym_expr.global_sym_decl->type.kind == TYPE_KIND_POINTER)
                            {
                                left_value = LLVMBuildLoad(module->builder, left_value, "ptrload");
                            }
                            break;
                        default:
                            RED_NOT_IMPLEMENTED;
                            break;
                    }
                    break;
                }
                default:
                    RED_NOT_IMPLEMENTED;
                    break;
            }

            IRExpression* right_expr = assign_st->right;
            LLVMValueRef right_value = llvm_gen_expression(context, module, ir_module, current_fn, right_expr, NULL);

            LLVMValueRef store = LLVMBuildStore(module->builder, right_value, left_value);
            return store;
        }
        case IR_ST_TYPE_LOOP_ST:
        {
            IRLoopStatement* loop_st = &st->loop_st;
            LLVMBasicBlockRef condition_block = LLVMAppendBasicBlockInContext(context, module->current_fn->proto->handle, "loop_condition");
            LLVMBasicBlockRef loop_block = LLVMAppendBasicBlockInContext(context,      module->current_fn->proto->handle, "loop_block");
            LLVMBasicBlockRef end_loop_block = LLVMAppendBasicBlockInContext(context,  module->current_fn->proto->handle, "end_loop_block");
            LLVMBuildBr(module->builder, condition_block);

            LLVMPositionBuilderAtEnd(module->builder, condition_block);
            LLVMValueRef condition_value = llvm_gen_expression(context, module, ir_module, current_fn, &loop_st->condition, NULL);
            LLVMBuildCondBr(module->builder, condition_value, loop_block, end_loop_block);

            LLVMPositionBuilderAtEnd(module->builder, loop_block);
            llvm_gen_compound_statement(context, module, ir_module, current_fn, &loop_st->body);
            // Jump back to the top of the loop
            LLVMBuildBr(module->builder, condition_block);

            // Let the builder in a position where code after the loop can be written to
            LLVMPositionBuilderAtEnd(module->builder, end_loop_block);

            return null;
        }
        case IR_ST_TYPE_FN_CALL_ST:
        {
            IRFunctionCallStatement* fn_call_st = &st->fn_call_st;
            LLVMValueRef fn_call_llvm = llvm_gen_fn_call(context, module, ir_module, current_fn, fn_call_st);
            return fn_call_llvm;
        }
        case IR_ST_TYPE_COMPOUND_ST:
            llvm_gen_compound_statement(context, module, ir_module, current_fn, &st->compound_st);
            return null;
        default:
            RED_NOT_IMPLEMENTED;
            return null;
    }
}

static inline TypeDeclarationLLVM llvm_gen_struct_type(LLVMContextRef context, ModuleContext* module, IRModule* ir_module, IRStructDecl* struct_decl)
{
    TypeDeclarationLLVM type_decl = ZERO_INIT;
    u32 field_count = struct_decl->field_count;
    IRFieldDecl* field_ptr = struct_decl->fields;
    for (u32 i = 0; i < field_count; i++)
    {
        IRFieldDecl* field = &field_ptr[i];
        llvm_type_append(&type_decl.child_types, llvm_gen_type(context, module, ir_module, &field->type));
    }
    // TODO: Anonymous structs vs named structs
    LLVMTypeRef type = LLVMStructCreateNamed(context, sb_ptr(&struct_decl->name));
    LLVMStructSetBody(type, type_decl.child_types.ptr, type_decl.child_types.len, false);
    type_decl.type = type;
    return type_decl;
}

static inline LLVMValueRef llvm_gen_global_sym(LLVMContextRef context, ModuleContext* module, IRModule* ir_module, IRSymDeclStatement* sym_decl, LLVMLinkage linkage)
{
    LLVMValueRef result = LLVMAddGlobal(module->handle, llvm_gen_type(context, module, ir_module, &sym_decl->type), sb_ptr(sym_decl->name));
    if (sym_decl->value.type != IR_EXPRESSION_TYPE_VOID)
    {
        LLVMSetInitializer(result, llvm_gen_expression(context, module, ir_module, NULL, &sym_decl->value, NULL));
    }
    else
    {
        LLVMSetInitializer(result, LLVMConstNull(llvm_gen_type(context, module, ir_module, &sym_decl->type)));
    }

    LLVMSetLinkage(result, linkage);
    LLVMSetGlobalConstant(result, sym_decl->is_const);
    LLVMSetVisibility(result, LLVMDefaultVisibility);

    return result;
}

static inline FnProtoLLVM llvm_gen_fn_proto(LLVMContextRef context, ModuleContext* module, IRModule* ir_module, IRFunctionPrototype* ir_proto)
{
    FnProtoLLVM proto = ZERO_INIT;
    proto.param_count = ir_proto->param_count;
    redassert(proto.param_count < MAX_PARAM_COUNT);

    for (u32 i = 0; i < proto.param_count; i++)
    {
        IRType* red_type = &ir_proto->params[i].type;
        proto.param_types[i] = llvm_gen_type(context, module, ir_module, red_type);
        if (module->debug.builder)
        {
            switch (red_type->kind)
            {
                case TYPE_KIND_PRIMITIVE:
                    proto.debug.param_types[i] = LLVMDIBuilderCreateBasicType(module->debug.builder, primitive_type_str(red_type->primitive_type), strlen(primitive_type_str(red_type->primitive_type)), red_type->size * 8, dwarf_encodings[red_type->primitive_type], 0);
                    redassert(proto.debug.param_types[i]);
                    break;
                default:
                    RED_NOT_IMPLEMENTED;
                    break;
            }
        }
        redassert(proto.param_types[i]);
    }

    proto.return_type = llvm_gen_type(context, module, ir_module, &ir_proto->ret_type);
    redassert(proto.return_type);
    proto.fn_type = LLVMFunctionType(proto.return_type, proto.param_types, proto.param_count, false);
    proto.handle = LLVMAddFunction(module->handle, sb_ptr(ir_proto->name), proto.fn_type);
    LLVMSetFunctionCallConv(proto.handle, LLVMCCallConv);
    LLVMSetLinkage(proto.handle, LLVMExternalLinkage);
    LLVMSetVisibility(proto.handle, LLVMDefaultVisibility);


    llvm_verify_function(proto.handle, "prototype", true);

    return proto;
}

static inline void llvm_gen_fn_definition(LLVMContextRef context, ModuleContext* module, IRModule* ir_module, IRFunctionDefinition* current_fn)
{
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(context, module->current_fn->proto->handle, "entry");
    module->builder = LLVMCreateBuilderInContext(context);
    LLVMPositionBuilderAtEnd(module->builder, entry);

    IRFunctionPrototype* ir_proto = current_fn->proto;
    u8 param_count = ir_proto->param_count;
    LLVMValueRef* params = null;
    IRParamDecl* ir_params = ir_proto->params;

    redassert(param_count < MAX_PARAM_COUNT);
    if (param_count)
    {
        redassert(ir_params);
        params = module->current_fn->param_array;
        LLVMGetParams(module->current_fn->proto->handle, params);

        for (usize i = 0; i < param_count; i++)
        {
            LLVMSetValueName(params[i], ir_params[i].name->ptr);
            module->current_fn->param_alloca_array[i] = LLVMBuildAlloca(module->builder, LLVMTypeOf(params[i]), "");
            LLVMBuildStore(module->builder, params[i], module->current_fn->param_alloca_array[i]);
        }
    }

    u32 st_count = current_fn->body.stmts.len;
    if (st_count > 0)
    {
        IRCompoundStatement* body = &current_fn->body;
        llvm_gen_compound_statement(context, module, ir_module, current_fn, body);
        if (!module->current_fn->return_already_emitted && current_fn->proto->ret_type.kind == TYPE_KIND_VOID)
        {
            LLVMBuildRetVoid(module->builder);
        }
    }
    else if (st_count == 0 && current_fn->proto->ret_type.kind == TYPE_KIND_VOID)
    {
        LLVMAppendBasicBlockInContext(context, module->current_fn->proto->handle, "entry");
        LLVMBuildRetVoid(module->builder);
    }
    else
    {
        RED_UNREACHABLE;
    }

    if (module->debug.builder)
    {
        LLVMDIFlags flags = LLVMDIFlagZero;
        // TODO: we need an extern decl for only prototyped functions for LLVMDIFlagPrototyped ????????????????????????
        flags |= LLVMDIFlagPublic;
        //flags |= LLVMDIFlagPrototyped;

        LLVMMetadataRef function_type = LLVMDIBuilderCreateSubroutineType(module->debug.builder, module->debug.file, module->current_fn->proto->debug.param_types, module->current_fn->proto->param_count, 0);
        LLVMMetadataRef di_function = LLVMDIBuilderCreateFunction(module->debug.builder, module->debug.file, sb_ptr(ir_proto->name), sb_len(ir_proto->name), sb_ptr(ir_proto->name), sb_len(ir_proto->name), module->debug.file, ir_proto->debug.line, function_type, false, true, ir_proto->debug.line, flags, false);
        LLVMSetSubprogram(module->current_fn->proto->handle, di_function);
    }
#if RED_LLVM_VERBOSE
    //llvm_verify_function(module->current_fn->proto->handle, "definition", false);
#endif
}

static inline TargetLLVM target_create(void)
{
    TargetLLVM target = ZERO_INIT;
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllTargets();
    LLVMInitializeAllAsmPrinters();
    LLVMInitializeAllAsmParsers();
    //LLVMInitializeAllDisassemblers();
    target.triple = LLVMGetDefaultTargetTriple();
    redassert(target.triple);

    char* error_message = null;
    if (LLVMGetTargetFromTriple(target.triple, &target.handle, &error_message))
    {
        os_exit_with_message("Couldn't create target: %s\n", error_message);
    }
    redassert(target.handle);

    LLVMCodeGenOptLevel opt_level = LLVMCodeGenLevelNone;
    LLVMRelocMode reloc_mode = LLVMRelocDefault;
    //const char* cpu = "generic";

    target.machine = LLVMCreateTargetMachine(target.handle, target.triple, "", "", opt_level, reloc_mode, LLVMCodeModelDefault);
    redassert(target.machine);

    if (!target.machine)
    {
        os_exit_with_message("Failed to create target machine\n");
    }

    target.data = LLVMCreateTargetDataLayout(target.machine);
    redassert(target.data);
    
    //char* target_triple = LLVMGetTargetMachineTriple(ctx.target_machine);

    //LLVMDisposeMessage(target_triple);

    return target;
}

static inline ModuleContext module_create(LLVMContextRef context, TargetLLVM target, IRModule* ir_module, const char* path, bool generate_debug_info, bool is_optimized)
{
    ModuleContext module = ZERO_INIT;
    module.handle = LLVMModuleCreateWithNameInContext(ir_module->name, context);
    LLVMSetModuleDataLayout(module.handle, target.data);
    LLVMSetSourceFileName(module.handle, path, strlen(path));
    LLVMSetTarget(module.handle, target.triple);

    if (generate_debug_info)
    {
        const char* flags = "";
        s32 runtime_version = 1;
        module.debug.builder = LLVMCreateDIBuilder(module.handle);
        module.debug.file = LLVMDIBuilderCreateFile(module.debug.builder, ir_module->name, strlen(ir_module->name), path, strlen(path));
        LLVMMetadataRef compile_unit = LLVMDIBuilderCreateCompileUnit(module.debug.builder, LLVMDWARFSourceLanguageC11, module.debug.file, "red", strlen("red"), is_optimized, flags, strlen(flags), runtime_version, "", 0, LLVMDWARFEmissionFull, 0, 0, 0, "", 0, "", 0);

    }

    return module;
}

static inline void llvm_register_primitive_types(LLVMContextRef context)
{
    llvm_primitive_types[IR_TYPE_PRIMITIVE_U8] = LLVMInt8TypeInContext(context);
    llvm_primitive_types[IR_TYPE_PRIMITIVE_U16] = LLVMInt16TypeInContext(context);
    llvm_primitive_types[IR_TYPE_PRIMITIVE_U32] = LLVMInt32TypeInContext(context);
    llvm_primitive_types[IR_TYPE_PRIMITIVE_U64] = LLVMInt64TypeInContext(context);
    llvm_primitive_types[IR_TYPE_PRIMITIVE_S8] = LLVMInt8TypeInContext(context);
    llvm_primitive_types[IR_TYPE_PRIMITIVE_S16] = LLVMInt16TypeInContext(context);
    llvm_primitive_types[IR_TYPE_PRIMITIVE_S32] = LLVMInt32TypeInContext(context);
    llvm_primitive_types[IR_TYPE_PRIMITIVE_S64] = LLVMInt64TypeInContext(context);
    llvm_primitive_types[IR_TYPE_PRIMITIVE_F32] = LLVMFloatTypeInContext(context);
    llvm_primitive_types[IR_TYPE_PRIMITIVE_F64] = LLVMDoubleTypeInContext(context);
    llvm_primitive_types[IR_TYPE_PRIMITIVE_F128] = LLVMFP128TypeInContext(context);
    llvm_primitive_types[IR_TYPE_PRIMITIVE_BOOL] = LLVMInt1TypeInContext(context);
}

bool llvm_gen_module_ir(LLVMContextRef context, ModuleContext* module, IRModule* ir_module)
{
    ExplicitTimer ir_dt = os_timer_start("IRGen");

    u32 module_count = ir_module->modules.len;
    IRModule* module_ptr = ir_module->modules.ptr;
    for (u32 i = 0; i < module_count; i++)
    {
        IRModule* module_it = &module_ptr[i];
        // Copy module name as prefix
        const char* module_prefix = module_it->name;
        // Copy main module name to child modules
        module_it->name = ir_module->name;

    }

    IRStructDeclBuffer* struct_decls = &ir_module->struct_decls;
    u64 struct_count = struct_decls->len;
    IRStructDecl* struct_decl_ptr = struct_decls->ptr;
    for (usize i = 0; i < struct_count; i++)
    {
        IRStructDecl* struct_decl = &struct_decl_ptr[i];
        llvm_type_decl_append(&module->type_declarations, llvm_gen_struct_type(context, module, ir_module, struct_decl));
    }

    IRSymDeclStatementBuffer* global_sym_decls = &ir_module->global_sym_decls;
    u64 global_sym_decl_count = global_sym_decls->len;
    IRSymDeclStatement* global_ptr = global_sym_decls->ptr;
    for (u64 i = 0; i < global_sym_decl_count; i++)
    {
        IRSymDeclStatement* sym_decl = &global_ptr[i];
        llvm_value_append(&module->global_sym_buffer, llvm_gen_global_sym(context, module, ir_module, sym_decl, LLVMExternalLinkage));
    }

    IRFunctionPrototypeBuffer* fn_proto_buffer = &ir_module->fn_prototypes;
    u64 fn_proto_count = fn_proto_buffer->len;
    IRFunctionPrototype* fn_proto_ptr = fn_proto_buffer->ptr;
    for (u64 i = 0; i < fn_proto_count; i++)
    {
        IRFunctionPrototype* fn_proto = &fn_proto_ptr[i];
        llvm_fn_proto_append(&module->fn_proto_buffer, llvm_gen_fn_proto(context, module, ir_module, fn_proto));
    }

    IRFunctionDefinitionBuffer* fn_defs = &ir_module->fn_definitions;
    u32 fn_def_count = fn_defs->len;
    for (usize i = 0; i < fn_def_count; i++)
    {
        CurrentFnLLVM current_fn = ZERO_INIT;
        IRFunctionDefinition* fn_def_it = &fn_defs->ptr[i];
        IRFunctionPrototype* fn_proto_ref = fn_def_it->proto;
        u64 proto_index = fn_proto_ref - fn_proto_ptr;
        module->current_fn = &current_fn;
        module->current_fn->proto = &module->fn_proto_buffer.ptr[proto_index];
        redassert(module->current_fn->proto);
        llvm_gen_fn_definition(context, module, ir_module, fn_def_it);
    }

    bool result = llvm_verify_module(module->handle);

    os_timer_end(&ir_dt);
    return result;
}

void llvm_gen_machine_code(IRModule* module_ir)
{
    ExplicitTimer llvm_init_dt = os_timer_start("MCI");
    TargetLLVM target = target_create();
    LLVMContextRef context = LLVMContextCreate();
    ModuleContext module = module_create(context, target, module_ir, "badpath->fixme", false, false);

    os_timer_end(&llvm_init_dt);

    llvm_register_primitive_types(context);
    LLVMAddModuleFlag(module.handle, LLVMModuleFlagBehaviorWarning, "CodeView", strlen("CodeView"), LLVMValueAsMetadata(LLVMConstInt(llvm_primitive_types[IR_TYPE_PRIMITIVE_U32], 1, false)));

    if (!llvm_gen_module_ir(context, &module, module_ir))
    {
        print("Could not generate LLVM IR\n");
        return;
    }
    else
    {
        print("LLVM IR generated successfully\n");
    }

    ExplicitTimer obj_gen_dt = os_timer_start("ObjWr");
    char* error_message = NULL;
    LLVMBool obj_gen_errors = LLVMTargetMachineEmitToFile(target.machine, module.handle, "red_module.obj", LLVMObjectFile, &error_message);
    if (obj_gen_errors)
    {
        print("\nError generating machine code: \n%s\n\n", error_message);
    }
    else
    {
        print("\nMachine code was generated successfully in %s\n\n", "red_module.obj");
    }
    os_timer_end(&obj_gen_dt);

    ExplicitTimer vs_sdk_find_dt = os_timer_start("VSSDK");
    Find_Result result = find_visual_studio_and_windows_sdk();
    //usize windows_sdk_root_len = wcslen(result.windows_sdk_root);
    usize windows_sdk_um_library_path_len = wcslen(result.windows_sdk_um_library_path);
    usize windows_sdk_ucrt_library_path_len = wcslen(result.windows_sdk_ucrt_library_path);
    //usize vs_exe_path_len = wcslen(result.vs_exe_path);
    usize vs_library_path_len = wcslen(result.vs_library_path);

    //SB* windows_sdk_root = sb_alloc();
    SB* windows_sdk_um_path = sb_alloc();
    sb_append_str(windows_sdk_um_path, "-libpath:");
    SB* windows_sdk_ucrt_path = sb_alloc();
    sb_append_str(windows_sdk_ucrt_path, "-libpath:");
    SB* vs_lib_path = sb_alloc();
    sb_append_str(vs_lib_path, "-libpath:");

    char buffer[512];
    u64 buffer_len;
    wcstombs(buffer, result.windows_sdk_um_library_path, windows_sdk_um_library_path_len);
    buffer[windows_sdk_um_library_path_len] = 0;
    redassert((buffer_len = strlen(buffer)) == windows_sdk_um_library_path_len);
    sb_append_str(windows_sdk_um_path, buffer);
    wcstombs(buffer, result.windows_sdk_ucrt_library_path, windows_sdk_ucrt_library_path_len);
    buffer[windows_sdk_ucrt_library_path_len] = 0;
    redassert((buffer_len = strlen(buffer)) == windows_sdk_ucrt_library_path_len);
    sb_append_str(windows_sdk_ucrt_path, buffer);
    wcstombs(buffer, result.vs_library_path, vs_library_path_len);
    buffer[vs_library_path_len] = 0;
    redassert((buffer_len = strlen(buffer)) == vs_library_path_len);
    sb_append_str(vs_lib_path, buffer);

    free_resources(&result);
    // TODO: Buggy shit. Find out what's going on
    redassert(!(windows_sdk_ucrt_path->ptr[sb_len(windows_sdk_ucrt_path) - 1] == 1));

    const char* linker_args[] =
    {
        "-subsystem:console", "/debug", "-out:red_module.exe", sb_ptr(windows_sdk_um_path), sb_ptr(windows_sdk_ucrt_path), sb_ptr(vs_lib_path), "red_module.obj", "libcmtd.lib", "libucrtd.lib"
    };
    os_timer_end(&vs_sdk_find_dt);

    print("Linker command:\n");
    for (s32 i = 0; i < array_length(linker_args); i++)
    {
        print("%s ", linker_args[i]);
    }
    print("\n\n");
    ExplicitTimer linker_dt = os_timer_start("Link");
    lld_linker_driver(linker_args, array_length(linker_args), LLD_BINARY_FORMAT_COFF);
    os_timer_end(&linker_dt);
}
