#include "compiler_types.h"
#include "llvm_codegen.h"
#include "os.h"



static void llvm_diagnostic_handler(LLVMDiagnosticInfoRef di, void* context)
{
    char* message = LLVMGetDiagInfoDescription(di);
    LLVMDiagnosticSeverity severity = LLVMGetDiagInfoSeverity(di);
    const char* severity_name = "unknown";
    LLVMContext* llvm_context = (LLVMContext*)context;
    switch (severity)
    {
        case LLVMDSError:
            print("LLVM error for %s: %s\n", "red_module", message);
            exit(1);
        case LLVMDSWarning:
            severity_name = "warning";
            break;
        case LLVMDSRemark:
            severity_name = "remark";
            break;
        case LLVMDSNote:
            severity_name = "note";
            break;
        default:
            RED_NOT_IMPLEMENTED;
            break;

    }

    print("LLVM %s: %s\n", severity_name, message);
    LLVMDisposeMessage(message);
}

LLVMSetup llvm_setup(LLVMContext* ctx)
{
    LLVMSetup llvm_setup = ZERO_INIT;
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllTargets();
    LLVMInitializeAllAsmPrinters();
    LLVMInitializeAllAsmParsers();

    llvm_setup.default_target_triple = LLVMGetDefaultTargetTriple();
    char* error_message = NULL;

    if (LLVMGetTargetFromTriple(llvm_setup.default_target_triple, &llvm_setup.target, &error_message) != false)
    {
        print("Couldn't create target: %s\n", error_message);
        exit(1);
    }

    s32 alloca_address_space = 0;
    print("Target set to %s\n", llvm_setup.default_target_triple);
    
    LLVMCodeGenOptLevel opt_level;
    LLVMRelocMode reloc_mode = LLVMRelocDefault;

    // not optimizations for now
    opt_level = LLVMCodeGenLevelNone;

    // generic cpu for now
    const char* cpu = "generic";

    // features

    llvm_setup.target_machine = LLVMCreateTargetMachine(llvm_setup.target, llvm_setup.default_target_triple, "", "", opt_level, reloc_mode, LLVMCodeModelDefault);
    if (!llvm_setup.target_machine)
    {
        print("Failed to create target machine\n");
        exit(1);
    }

    llvm_setup.target_data_layout = LLVMCreateTargetDataLayout(llvm_setup.target_machine);

    char* target_triple = LLVMGetTargetMachineTriple(llvm_setup.target_machine);

    char* arch_name = strtok(target_triple, "-");
    char* vendor_name = strtok(NULL, "-");
    char* os_name = strtok(null, "0123456789");

    LLVMDisposeMessage(target_triple);

    /* own types for build options */
    alloca_address_space = 0;

    llvm_setup.context = LLVMContextCreate();
    //LLVMContextSetDiagnosticHandler(llvm_setup.context, llvm_diagnostic_handler, ctx);

    return llvm_setup;
}

typedef struct LLVMTypeInfo
{
    LLVMTypeRef u8_type;
    LLVMTypeRef u16_type;
    LLVMTypeRef u32_type;
    LLVMTypeRef u64_type;

    LLVMTypeRef s8_type;
    LLVMTypeRef s16_type;
    LLVMTypeRef s32_type;
    LLVMTypeRef s64_type;

    LLVMTypeRef f32_type;
    LLVMTypeRef f64_type;
    LLVMTypeRef f128_type;

    LLVMTypeRef pointer_type;
} LLVMTypeInfo;

typedef struct LLVMAlignInfo
{
    u8 u8_align;
    u8 u16_align;
    u8 u32_align;
    u8 u64_align;

    u8 s8_align;
    u8 s16_align;
    u8 s32_align;
    u8 s64_align;

    u8 f32_align;
    u8 f64_align;
    u8 f128_align;

    u8 pointer_align;
    
    bool little_endian;
} LLVMAlignInfo;

static LLVMTypeInfo llvm_type_info;
static LLVMAlignInfo llvm_align_info;

//void llvm_register_types(LLVMTypeInfo* type_info, LLVMAlignInfo* align_info)
//{
//    type_info->u8_type  = LLVMIntTypeInContext(llvm_setup.context, 8);
//    type_info->u16_type = LLVMIntTypeInContext(llvm_setup.context, 16);
//    type_info->u32_type = LLVMIntTypeInContext(llvm_setup.context, 32);
//    type_info->u64_type = LLVMIntTypeInContext(llvm_setup.context, 64);
//
//    type_info->s8_type  = LLVMIntTypeInContext(llvm_setup.context, 8);
//    type_info->s16_type = LLVMIntTypeInContext(llvm_setup.context, 16);
//    type_info->s32_type = LLVMIntTypeInContext(llvm_setup.context, 32);
//    type_info->s64_type = LLVMIntTypeInContext(llvm_setup.context, 64);
//
//    type_info->f32_type  = LLVMFloatTypeInContext(llvm_setup.context);
//    type_info->f64_type  = LLVMDoubleTypeInContext(llvm_setup.context);
//    type_info->f128_type = LLVMFP128TypeInContext(llvm_setup.context);
//
//    type_info->pointer_type = LLVMPointerType(type_info->s32_type, 0);
//
//    align_info->s8_align  = LLVMABIAlignmentOfType(llvm_setup.target_data_layout, type_info->s8_type);
//    align_info->s16_align = LLVMABIAlignmentOfType(llvm_setup.target_data_layout, type_info->s16_type);
//    align_info->s32_align = LLVMABIAlignmentOfType(llvm_setup.target_data_layout, type_info->s32_type);
//    align_info->s64_align = LLVMABIAlignmentOfType(llvm_setup.target_data_layout, type_info->s64_type);
//
//    align_info->u8_align  = LLVMABIAlignmentOfType(llvm_setup.target_data_layout, type_info->u8_type);
//    align_info->u16_align = LLVMABIAlignmentOfType(llvm_setup.target_data_layout, type_info->u16_type);
//    align_info->u32_align = LLVMABIAlignmentOfType(llvm_setup.target_data_layout, type_info->u32_type);
//    align_info->u64_align = LLVMABIAlignmentOfType(llvm_setup.target_data_layout, type_info->u64_type);
//
//    align_info->f32_align  = LLVMABIAlignmentOfType(llvm_setup.target_data_layout, type_info->f32_type);
//    align_info->f64_align  = LLVMABIAlignmentOfType(llvm_setup.target_data_layout, type_info->f64_type);
//    align_info->f128_align = LLVMABIAlignmentOfType(llvm_setup.target_data_layout, type_info->f128_type);
//
//    align_info->pointer_align = LLVMABIAlignmentOfType(llvm_setup.target_data_layout, type_info->pointer_type);
//
//    align_info->little_endian = LLVMByteOrder(llvm_setup.target_data_layout) == LLVMLittleEndian;
//    redassert(align_info->little_endian);
//}
//
//LLVMTypeRef llvm_get_node_type(Node* node)
//{
//    redassert(node->bn.n_id == TYPE);
//
//    return (LLVMTypeRef) node->bn.n_backend_ref;
//}
//
//LLVMTypeRef llvm_primitive_type(Node* node)
//{
//    redassert(node->bn.n_id == TYPE);
//    RedType* red_type = node->type_expr.resolved_type;
//    redassert(red_type->kind == PRIMITIVE);
//
//    if (node->bn.n_backend_ref)
//    {
//        return node->bn.n_backend_ref;
//    }
//
//    switch (red_type->primitive)
//    {
//        case RED_TYPE_PRIMITIVE_U8:
//            RED_NOT_IMPLEMENTED;
//            break;
//        case RED_TYPE_PRIMITIVE_U16:
//            RED_NOT_IMPLEMENTED;
//            break;
//        case RED_TYPE_PRIMITIVE_U32:
//            RED_NOT_IMPLEMENTED;
//            break;
//        case RED_TYPE_PRIMITIVE_U64:
//            RED_NOT_IMPLEMENTED;
//            break;
//        case RED_TYPE_PRIMITIVE_S8:
//            RED_NOT_IMPLEMENTED;
//            break;
//        case RED_TYPE_PRIMITIVE_S16:
//            RED_NOT_IMPLEMENTED;
//            break;
//        case RED_TYPE_PRIMITIVE_S32:
//            node->bn.n_backend_ref = llvm_type_info.s32_type;
//            return node->bn.n_backend_ref;
//        case RED_TYPE_PRIMITIVE_S64:
//            RED_NOT_IMPLEMENTED;
//            break;
//        case RED_TYPE_PRIMITIVE_F32:
//            RED_NOT_IMPLEMENTED;
//            break;
//        case RED_TYPE_PRIMITIVE_F64:
//            RED_NOT_IMPLEMENTED;
//            break;
//        case RED_TYPE_PRIMITIVE_F128:
//            RED_NOT_IMPLEMENTED;
//            break;
//        default:
//            RED_NOT_IMPLEMENTED;
//            break;
//    }
//
//    return null;
//}
//
//LLVMTypeRef llvm_fn_ret_type(LLVMContextRef ctx, Node* node)
//{
//    redassert(node->bn.n_id == TYPE);
//    RedType* red_type = node->type_expr.resolved_type;
//    if (red_type)
//    {
//        return llvm_primitive_type(node);
//    }
//    else
//    {
//        return LLVMVoidTypeInContext(ctx);
//    }
//}
//
//LLVMTypeRef llvm_fn_type(LLVMContextRef ctx, Node* node)
//{
//    redassert(node->bn.n_id == FN_PROTO);
//    if (node->bn.n_backend_ref)
//    {
//        return node->bn.n_backend_ref;
//    }
//
//    LLVMTypeRef llvm_param_types[100];
//    u32 param_count = node->fn_proto.params.len;
//    LLVMTypeRef llvm_ret_type = llvm_fn_ret_type(ctx, node->fn_proto.ret_type);
//    redassert(llvm_ret_type);
//
//    for (u32 i = 0; i < param_count; i++)
//    {
//        Node* param_type_node = node->fn_proto.params.ptr[i]->param_decl.type;
//        llvm_param_types[i] = llvm_primitive_type(param_type_node);
//        redassert(llvm_param_types[i]);
//    }
//    node->bn.n_backend_ref = LLVMFunctionType(llvm_ret_type, llvm_param_types, param_count, false);
//    return node->bn.n_backend_ref;
//}
//
//LLVMTypeRef llvm_type(LLVMContext* ctx, Node* node)
//{
//    if (node->bn.n_id == FN_PROTO)
//    {
//        return llvm_fn_type(ctx->context, node);
//    }
//    else
//    {
//        // TODO: We should consider complex types in the future
//        return llvm_primitive_type(node);
//    }
//
//    RED_NOT_IMPLEMENTED;
//    return null;
//}
//
//static inline const char* get_fn_proto_name(Node* proto)
//{
//    return sb_ptr(proto->fn_proto.sym->sym_expr.name);
//}
//
//static inline bool gen_node_type_equal(Node* node, ASTType type)
//{
//    return node->bn.n_id == type;
//}
//
//void gen_emit_implicit_return(LLVMContext* ctx)
//{
//    LLVMBuildRetVoid(ctx->builder);
//}
//
//LLVMValueRef gen_fn_proto(LLVMContext* ctx, Node* proto)
//{
//    // TODO: we should resolve this
//    LLVMValueRef function = LLVMAddFunction(ctx->module, get_fn_proto_name(proto), llvm_type(ctx, proto));
//    // TODO: we are skipping function attributes for now
//    // TODO: callconv __cdecl (VARARGS) vs __stdcall (NO VARARGS). Windows only? for win32 api calls we have to modify this
//    LLVMSetFunctionCallConv(function, LLVMCCallConv);
//
//    // TODO: for now, all functions have external linkage (see types of linking)
//    LLVMSetLinkage(function, LLVMExternalLinkage);
//    LLVMSetVisibility(function, LLVMDefaultVisibility);
//    
//    // TODO: Debug info -> omit this can be error prone, although I doubt it
//
//    return function;
//}
//
//LLVMValueRef gen_emit_alloca(LLVMContext* ctx, LLVMTypeRef type, const char* name)
//{
//    LLVMBasicBlockRef current_block = LLVMGetInsertBlock(ctx->builder);
//    LLVMPositionBuilderBefore(ctx->builder, ctx->alloca_point);
//    LLVMValueRef alloca = LLVMBuildAlloca(ctx->builder, type, name);
//    LLVMPositionBuilderAtEnd(ctx->builder, current_block);
//
//    return alloca;
//}
//
//LLVMValueRef decl_ref(Node* node)
//{
//    // TODO: alias? what does that mean in codegen?
//    return node->bn.n_backend_ref;
//}
//
//static inline void gen_emit_store(LLVMContext* ctx, Node* node, LLVMValueRef value)
//{
//    LLVMBuildStore(ctx->builder, value, decl_ref(node));
//}
//
//void gen_emit_parameter(LLVMContext* ctx, Node* decl, u32 index)
//{
//    redassert(gen_node_type_equal(decl, PARAM_DECL));
//
//    const char* name = decl->param_decl.sym ? decl->param_decl.sym->sym_expr.name->ptr : "anon";
//    Node* type_node = decl->param_decl.type;
//    type_node->bn.n_backend_ref = gen_emit_alloca(ctx, llvm_type(ctx, type_node), name);
//
//    // TODO: debug
//    LLVMValueRef llvm_fn_param = LLVMGetParam(ctx->function, index);
//    redassert(llvm_fn_param);
//    gen_emit_store(ctx, decl, llvm_fn_param);
//}
//
//LLVMValueRef gen_emit_expr(LLVMContext* ctx, Node* node)
//{
//    RED_NOT_IMPLEMENTED;
//    return null;
//}
//
//LLVMBasicBlockRef gen_create_free_block(LLVMContext* ctx, const char* block_name)
//{
//    return LLVMCreateBasicBlockInContext(ctx->context, block_name);
//}
//
//void gen_emit_block(LLVMContext* ctx, LLVMBasicBlockRef next_block)
//{
//    redassert(ctx->current_block == NULL);
//    LLVMAppendExistingBasicBlock(ctx->function, next_block);
//    LLVMPositionBuilderAtEnd(ctx->builder, next_block);
//    ctx->current_block = next_block;
//    ctx->current_block_is_target = false;
//}
//
//bool gen_check_block_branch_emit(LLVMContext* ctx)
//{
//    if (!ctx->current_block)
//    {
//        return false;
//    }
//
//	// If it's not used, we can delete the previous block and skip the branch.
//	// Unless it is the entry block or a label target for jumps
//	// These empty blocks will occur when doing branches.
//	// Consider:
//	// while (1)
//	// {
//	//   break;
//	//   break;
//	// }
//	// Naively we'd output
//	// br label %for.cond  - 1st break
//	// br label %for.cond  - 2nd break
//	// br label %for.cond  - end of scope
//	//
//	// The fix is to introduce a new block after a break:
//	// br label %for.cond
//	// jmp:
//	// br label %for.cond
//	// jmp.1:
//	// br label %for.cond
//	//
//	// But this leaves us with blocks that have no parent.
//	// Consequently we will delete those and realize that
//	// we then have no need for emitting a br.
//
//    if (!ctx->current_block_is_target && !LLVMGetFirstUse(LLVMBasicBlockAsValue(ctx->current_block)))
//    {
//        LLVMDeleteBasicBlock(ctx->current_block);
//        ctx->current_block = NULL;
//        return false;
//    }
//    
//    return true;
//}
//
//void gen_emit_branch(LLVMContext* ctx, LLVMBasicBlockRef next_block)
//{
//    if (!gen_check_block_branch_emit(ctx))
//    {
//        return;
//    }
//
//    ctx->current_block = NULL;
//    LLVMBuildBr(ctx->builder, next_block);
//}
//
//void gen_emit_jmp(LLVMContext* ctx, LLVMBasicBlockRef block)
//{
//    gen_emit_branch(ctx, block);
//    LLVMBasicBlockRef post_jump_block = gen_create_free_block(ctx, "jmp");
//    gen_emit_block(ctx, post_jump_block);
//}
//
//void gen_emit_return(LLVMContext* ctx, Node* node)
//{
//	// TODO: Ensure we are on a branch that is non empty.
//
//    bool in_expression_block = ctx->expr_block_exit != NULL;
//
//    // push error
//
//    LLVMValueRef ret_value = node->return_expr.expr ? gen_emit_expr(ctx, node->return_expr.expr) : NULL;
//    // pop error
//
//    if (ctx->expr_block_exit)
//    {
//        if (ctx->return_out)
//        {
//            LLVMBuildStore(ctx->builder, ret_value, ctx->return_out);
//        }
//        gen_emit_jmp(ctx, ctx->expr_block_exit);
//        return;
//    }
//
//    if (!ret_value)
//    {
//        gen_emit_implicit_return(ctx);
//    }
//    else
//    {
//        if (ctx->return_out)
//        {
//            LLVMBuildStore(ctx->builder, ret_value, ctx->return_out);
//            gen_emit_implicit_return(ctx);
//        }
//        else
//        {
//            LLVMBuildRet(ctx->builder, ret_value);
//        }
//    }
//
//    ctx->current_block = NULL;
//
//    LLVMBasicBlockRef post_ret_block = gen_create_free_block(ctx, "ret");
//    gen_emit_block(ctx, post_ret_block);
//}
//
//
//void gen_emit_statement(LLVMContext* ctx, Node* statement)
//{
//    switch (statement->bn.n_id)
//    {
//        case RETURN_EXPR:
//            gen_emit_return(ctx, statement);
//            break;
//        default:
//            break;
//    }
//}
//
//void gen_fn_body(LLVMContext* ctx, Node* decl)
//{
//    // TODO: emit debug
//    LLVMValueRef prev_function = ctx->function;
//    LLVMBuilderRef prev_builder = ctx->builder;
//
//    ctx->function = decl->fn_def.proto->bn.n_backend_ref;
//
//    // TODO: debug
//
//    ctx->current_fn_decl = decl;
//
//    //LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(ctx->context, ctx->function, "entry");
//    LLVMBasicBlockRef entry = LLVMCreateBasicBlockInContext(ctx->context, "entry");
//    ctx->current_block = entry;
//    ctx->current_block_is_target = true;
//    ctx->expr_block_exit = NULL;
//    ctx->builder = LLVMCreateBuilderInContext(ctx->context);
//    LLVMPositionBuilderAtEnd(ctx->builder, entry);
//
//    // TODO: get info about: Why int32?
//    //LLVMTypeRef int32_type = LLVMInt32TypeInContext(ctx->context);
//    //LLVMValueRef alloca_point = LLVMBuildAlloca(ctx->builder, int32_type, "alloca_point");
//    //ctx->alloca_point = alloca_point;
//    
//    FnProto* proto = &decl->fn_def.proto->fn_proto;
//
//    s32 arg = 0;
//
//    // TODO: is not void
//    if (proto->ret_type)
//    {
//        ctx->return_out = LLVMGetParam(ctx->function, arg++);
//    }
//    else
//    {
//        ctx->return_out = NULL;
//    }
//
//    // TODO: debug
//
//    for (s32 i = 0; i < proto->params.len; i++)
//    {
//        gen_emit_parameter(ctx, decl->fn_def.proto->fn_proto.params.ptr[i], arg++);
//    }
//
//    // TODO: debug
//
//    Node* body = decl->fn_def.body;
//    redassert(gen_node_type_equal(body, COMPOUND_STATEMENT));
//    NodeBuffer* body_statements = &body->compound_statement.statements;
//
//    for (s32 i = 0; i < body_statements->len; i++)
//    {
//        Node* statement = body_statements->ptr[i];
//        gen_emit_statement(ctx, statement);
//    }
//
//    if (ctx->current_block && !LLVMGetFirstInstruction(ctx->current_block) && !LLVMGetFirstUse(LLVMBasicBlockAsValue(ctx->current_block)))
//    {
//        LLVMBasicBlockRef prev_block = LLVMGetPreviousBasicBlock(ctx->current_block);
//        LLVMDeleteBasicBlock(ctx->current_block);
//        ctx->current_block = prev_block;
//        LLVMPositionBuilderAtEnd(ctx->builder, ctx->current_block);
//    }
//
//    if (ctx->current_block && !LLVMGetBasicBlockTerminator(ctx->current_block))
//    {
//        // defer stuff
//        // TODO: emit "implicit" (what does it mean?) return
//    }
//
//    //if (LLVMGetInstructionParent(alloca_point))
//    //{
//    //    ctx->alloca_point = NULL;
//    //    LLVMInstructionEraseFromParent(alloca_point);
//    //}
//
//    LLVMDisposeBuilder(ctx->builder);
//
//    // TODO: debug
//
//    ctx->builder = prev_builder;
//    ctx->function = prev_function;
//}
//
//void gen_fn_definitions(LLVMContext* ctx, NodeBuffer* fn_definitions)
//{
//    size_t fn_count = fn_definitions->len;
//    for (s32 i = 0; i < fn_count; i++)
//    {
//        Node* fn_node = fn_definitions->ptr[i];
//        Node* fn_proto = fn_node->fn_def.proto;
//        LLVMValueRef fn_proto_llvm = gen_fn_proto(ctx, fn_proto);
//        gen_fn_body(ctx, fn_node);
//        if (LLVMVerifyFunction(fn_proto_llvm, LLVMPrintMessageAction))
//        {
//            print("Failed to verify function\n");
//        }
//        print("LLVM fn proto: %s\n", LLVMPrintValueToString(fn_proto_llvm));
//    }
//}

LLVMModuleRef llvm_setup_module(const char* module_name, const char* full_path, LLVMSetup* llvm_setup)
{
    LLVMModuleRef module = LLVMModuleCreateWithNameInContext(module_name, llvm_setup->context);
    LLVMSetModuleDataLayout(module, llvm_setup->target_data_layout);
    LLVMSetSourceFileName(module, full_path, strlen(full_path));
    LLVMSetTarget(module, llvm_setup->default_target_triple);

    return module;
}

void llvm_codegen(RedModuleTree* mod_ast)
{
    //LLVMContext ctx = { 0 };
    //LLVMSetup llvm_setup = llvm_target_setup(&ctx);
    //llvm_register_types(&llvm_type_info, &llvm_align_info);
    //
    //// TODO: mangle module name
    //char* module_name = "red_module";
    //char* full_path = "test.red";

    //LLVMModuleRef module = llvm_setup_module(module_name, full_path, &llvm_setup);

    //// TODO: debug info setup

    //ctx.context = llvm_setup.context;
    //ctx.module = module;

    //gen_fn_definitions(&ctx, &mod_ast->fn_definitions);

    //char* message = null;
    //if (LLVMVerifyModule(ctx.module, LLVMPrintMessageAction, &message))
    //{
    //    if (*message)
    //    {
    //        print("LLVM module failed to verify: %s\n", message);
    //    }
    //    else
    //    {
    //        print("LLVM module failed to verify!\n");
    //    }
    //}

    //char* module_str = LLVMPrintModuleToString(ctx.module);
    //if (!module_str)
    //{
    //    print("LLVM module failed to be printed!\n");
    //}
    //else
    //{
    //    print("\nLLVM module\n\n%s\n\n", module_str);
    //}
}