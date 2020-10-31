#include "llvm_codegen.h"
#include "os.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <stdarg.h>

void log_error(const char* format, ...)
{
    fprintf(stdout, "[CodeGen ERROR] ");
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    fputc('\n', stdout);
    va_end(args);
}

using namespace llvm;
using namespace RedAST;
using FnMap = LLVMMap<llvm::Function>;
using ValueMap = LLVMMap<llvm::Value>;
using TypeMap = LLVMMap<llvm::Type>;

static LLVMContext* context;
namespace RedIR
{
    struct Context
    {
        Module* module;
        IRBuilder<>* builder;
        FnMap function_map;
        ValueMap value_map;
        TypeMap type_map;

        Context(const char* module_name)
        {
            context = new LLVMContext();
            module = new Module(module_name, *context);
            builder = new IRBuilder<>(*context);
            function_map.count = 0;
            value_map.count = 0;
            type_map.count = 0;
        }
    };
}

using namespace RedIR;

Value* VariableExpr::codegen(Context* ctx)
{
    RED_NOT_IMPLEMENTED;
    return nullptr;
}

Value* FloatExpr::codegen(Context* ctx)
{
    RED_NOT_IMPLEMENTED;
    return nullptr;
}

Value* BoolExpr::codegen(Context* ctx)
{
    RED_NOT_IMPLEMENTED;
    return nullptr;
}

Value* BlockExpr::codegen(Context* ctx)
{
    auto* statements = this->expressions_in_block;
    assert(statements->size() == 1);
    Value* value = statements->at(0)->codegen(ctx);
    return value;
}

Value* IntExpr::codegen(Context* ctx)
{
    ConstantInt* integer = nullptr;
    if (this->bigint->digit_count > 1)
    {
        RED_NOT_IMPLEMENTED;
    }
    else
    {
        assert(this->bigint->digit_count == 1);
        integer = ConstantInt::get(*context, APInt(32, this->bigint->digit, false));
    }
    return integer;
}

Value* BinaryExpr::codegen(Context* ctx)
{
    Value* left = this->left->codegen(ctx);
    Value* right = this->right->codegen(ctx);
    if (!left || !right)
    {
        return nullptr;
    }

    if (!token_is_binop(this->op))
    {
        return nullptr;
    }

    switch (this->op)
    {
        case TOKEN_ID_CMP_EQ:
            return ctx->builder->CreateICmpEQ(left, right, "cmptmp");
        case TOKEN_ID_CMP_NOT_EQ:
            return ctx->builder->CreateICmpNE(left, right, "cmptmp");
        case TOKEN_ID_CMP_LESS:
            return ctx->builder->CreateICmpSLT(left, right, "cmptmp");
        case TOKEN_ID_CMP_GREATER:
            return ctx->builder->CreateICmpSGT(left, right, "cmptmp");
        case TOKEN_ID_CMP_LESS_OR_EQ:
            return ctx->builder->CreateICmpSLE(left, right, "cmptmp");
        case TOKEN_ID_CMP_GREATER_OR_EQ:
            return ctx->builder->CreateICmpSGE(left, right, "cmptmp");
        default:
            RED_NOT_IMPLEMENTED;
            return nullptr;
    }
}

Value* BranchExpr::codegen(Context* ctx)
{
    Value* cond_value = this->condition->codegen(ctx);
    if (!cond_value)
    {
        return nullptr;
    }

    Value* bool_cond = ctx->builder->CreateICmpNE(cond_value, ConstantInt::get(*context, APInt(1, 0, true)), "cond");
    llvm::Function* function = ctx->builder->GetInsertBlock()->getParent();

    BasicBlock* true_block = BasicBlock::Create(*context, "if-block", function);
    BasicBlock* false_block = BasicBlock::Create(*context, "else-block");
    BasicBlock* merge_block = BasicBlock::Create(*context, "merge-block");

    ctx->builder->CreateCondBr(bool_cond, true_block, false_block);

    ctx->builder->SetInsertPoint(true_block);

    Value* true_block_value = this->true_block->codegen(ctx);
    if (!true_block_value)
    {
        return nullptr;
    }
    
    ctx->builder->CreateBr(merge_block);
    true_block = ctx->builder->GetInsertBlock();

    function->getBasicBlockList().push_back(false_block);
    ctx->builder->SetInsertPoint(false_block);

    Value* false_block_value = this->false_block->codegen(ctx);
    // TODO: flexibility (not always we encounter else blocks
    if (!false_block_value)
    {
        return nullptr;
    }

    ctx->builder->CreateBr(merge_block);
    false_block = ctx->builder->GetInsertBlock();

    function->getBasicBlockList().push_back(merge_block);
    ctx->builder->SetInsertPoint(merge_block);
    PHINode* pn = ctx->builder->CreatePHI(Type::getInt32Ty(*context), 2, "iftmp");

    pn->addIncoming(true_block_value, true_block);
    pn->addIncoming(false_block_value, false_block);

    return pn;
}

llvm::Value* ReturnExpr::codegen(Context* ctx)
{
    return this->return_expr->codegen(ctx);
}

llvm::Function* RedAST::Prototype::codegen(Context* ctx)
{
    Buffer* return_type_name = this->return_type;
    Type* return_type = ctx->type_map.find_value(return_type_name);
    if (!return_type)
    {
        log_error("return type %s not found", buf_ptr(return_type_name));
        return nullptr;
    }

    Buffer* fn_name = this->name;

    std::vector<VariableExpr*>* parameters = &this->args;
    std::vector<Type*> llvm_args;
    FunctionType* fn_type;

    if (!parameters->empty())
    {
        for (usize i = 0; i < parameters->size(); i++)
        {
            VariableExpr* parameter = parameters->at(i);
            Buffer* type = parameter->type;
            Type* llvm_type = ctx->type_map.find_value(type);
            if (!llvm_type)
            {
                log_error("type %s not found", buf_ptr(type));
                return nullptr;
            }
        }
        fn_type = FunctionType::get(return_type, llvm_args, false); // varargs always false
    }
    else
    {
        fn_type = FunctionType::get(return_type, false);
    }

    llvm::Function* function = llvm::Function::Create(fn_type, llvm::Function::ExternalLinkage, buf_ptr(fn_name), ctx->module);

    if (!parameters->empty())
    {
        auto args = function->args();
        usize i = 0;
        for (auto& arg : args)
        {
            VariableExpr* parameter = static_cast<VariableExpr*>(parameters->at(i));
            arg.setName(buf_ptr(parameter->name));
            i++;
        }
    }

    /* TODO: register fn */
    return function;
}

llvm::Function* RedAST::Function::codegen(Context* ctx)
{
    Buffer* name = this->proto->name;
    llvm::Function* search_result = ctx->function_map.find_value(name);
    redassert(!search_result);

    llvm::Function* function = ctx->module->getFunction(buf_ptr(name));
    if (!function)
    {
        function = this->proto->codegen(ctx);
    }

    if (!function)
    {
        log_error("could not generate function %s prototype", buf_ptr(name));
        return nullptr;
    }

    if (!function->empty())
    {
        log_error("function %s cannot be redefined", buf_ptr(name));
        return nullptr;
    }

    BasicBlock* bb = BasicBlock::Create(*context, "entry", function);
    ctx->builder->SetInsertPoint(bb);

    ctx->value_map.clear();
    auto args = function->args();
    for (const Argument& arg : args)
    {
        Buffer bf = {};
        buf_init_from_str(&bf, arg.getName().data());
        ctx->value_map.append(&bf, (Value*)&arg);
    }

    Value* return_value = this->body->codegen(ctx);
    if (return_value)
    {
        ctx->builder->CreateRet(return_value);

        bool errors = verifyFunction(*function, &llvm::errs());
        if (errors)
        {
            os_exit(1);
        }

        return function;
    }

    function->eraseFromParent();
    return nullptr;
}

namespace RedIR
{
    static inline void register_types(TypeMap& map)
    {
        const char* typenames[] =
        {
            "s32",
        };

        llvm::Type* types[] =
        {
            llvm::Type::getInt32Ty(*context),
        };

        static_assert(array_length(typenames) == array_length(types), "Array length mismatch in types");

        for (u32 i = 0; i < array_length(typenames); i++)
        {
            Buffer* buf = buf_alloc();
            buf_init_from_str(buf, typenames[i]);
            map.append(buf, types[i]);
        }
    }
}

#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/Host.h>
#include <llvm/IR/LegacyPassManager.h>

namespace RedMachineCodeGen
{
    static void output_obj_file(Module* module)
    {
        ScopeTimer codegen_time("CG dt");
        auto target_triple = sys::getDefaultTargetTriple();

        InitializeAllTargetInfos();
        InitializeAllTargets();
        InitializeAllTargetMCs();
        InitializeAllAsmParsers();
        InitializeAllAsmPrinters();

        std::string error;
        auto target = TargetRegistry::lookupTarget(target_triple, error);

        if (!target)
        {
            errs() << error;
            exit(1);
        }

        auto cpu = "generic";
        auto features = "";

        TargetOptions options;
        auto rm = llvm::Optional<Reloc::Model>();
        auto target_machine = target->createTargetMachine(target_triple, cpu, features, options, rm);
        module->setDataLayout(target_machine->createDataLayout());

        auto filename = "output.o";
        std::error_code ec;
        raw_fd_ostream dest(filename, ec, sys::fs::OpenFlags(0));
        if (ec)
        {
            errs() << "Could not open file: " << ec.message();
            exit(1);
        }

        legacy::PassManager pass;
        if (target_machine->addPassesToEmitFile(pass, dest, nullptr, CGFT_ObjectFile))
        {
            errs() << "The target machine can't emit a file of this type";
            exit(1);
        }

        ExplicitTimer pass_run("Pass");
        pass.run(*module);
        dest.flush();
        pass_run.end();

        // outs() << "Wrote " << filename << "\n";
    }
}

void llvm_codegen(List<RedAST::Function*>* fn_list)
{
    ScopeTimer ir_plus_codegen("IR+CG");
    RedIR::Context context("red_module");
    
    RedIR::register_types(context.type_map);
    ExplicitTimer llvm_ir_generation("IRLLV");
    for (u32 i = 0; i < fn_list->length; i++)
    {
        RedAST::Function* ast_fn = fn_list->at(i);
        ast_fn->codegen(&context);
    }
    llvm_ir_generation.end();

    RedMachineCodeGen::output_obj_file(context.module);
}