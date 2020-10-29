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
    Value* int_expr(IntExpr* node)
    {
        ConstantInt* integer = nullptr;
        if (node->bigint->digit_count > 1)
        {
            RED_NOT_IMPLEMENTED;
        }
        else
        {
            assert(node->bigint->digit_count == 1);
            integer = ConstantInt::get(*context, APInt(32, node->bigint->digit, false));
        }
        return integer;
    }

    //Value* symbol_expr(SymbolExpr* node, LLVMMap<Value> value_map)
    //{
    //    /* Posible values
    //    - Function
    //    - Variable
    //    */
    //    Value* value = value_map.find_value(node->name);
    //    if (!value)
    //    {
    //        log_error("unknown variable name");
    //    }
    //    return value;
    //}

    //// TODO: continue
    //Value* call_expr(Expression* node, Module* module, IRBuilder<>* builder)
    //{
    //    redassert(node->type == NODE_TYPE_FN_CALL_EXPR);
    //    Function* callee_function = module->getFunction(buf_ptr(node->fn_call_expr.name));
    //    if (!callee_function)
    //    {
    //        log_error("unknown referenced function");
    //        return nullptr;
    //    }

    //    usize expected_arg_count = callee_function->arg_size();
    //    usize actual_arg_count = node->fn_call_expr.parameters.length;
    //    if (expected_arg_count != actual_arg_count)
    //    {
    //        log_error("expected %zu arguments, have %zu", expected_arg_count, actual_arg_count);
    //        return nullptr;
    //    }

    //    std::vector<Value*> args;
    //    //List<Expression*>* actual_args = &node->fn_call_expr.parameters;
    //    //for (usize i = 0; i != actual_args->length; i++)
    //    //{
    //    //    args.append()
    //    //}
    //    
    //    return builder->CreateCall(callee_function, args, "calltmp");
    //}

    llvm::Function* fn_prototype(RedAST::Prototype* node, LLVMMap<Type> type_map, Module* module)
    {
        Buffer* return_type_name = node->return_type;
        Type* return_type = type_map.find_value(return_type_name);
        if (!return_type)
        {
            log_error("return type %s not found", buf_ptr(return_type_name));
            return nullptr;
        }

        Buffer* fn_name = node->name;

        std::vector<Expression*>* parameters = &node->args;
        std::vector<Type*> llvm_args;
        FunctionType* fn_type;

        if (!parameters->empty())
        {
            for (usize i = 0; i < parameters->size(); i++)
            {
                VariableExpr* parameter = static_cast<VariableExpr*>(parameters->at(i));
                Buffer* type = parameter->type;
                Type* llvm_type = type_map.find_value(type);
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

        llvm::Function* function = llvm::Function::Create(fn_type, llvm::Function::ExternalLinkage, buf_ptr(fn_name), module);

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

    static inline llvm::Value* fn_return_expr(RedAST::ReturnExpr* return_expr)
    {
        redassert(return_expr->return_expr);
        // TODO: fix
        IntExpr* return_value = static_cast<IntExpr*>(return_expr->return_expr);
        redassert(return_value->bigint->digit_count == 1);
        llvm::Value* value = int_expr(return_value);

        return value;
    }
    

    llvm::Function* fn_definition(RedAST::Function* node, LLVMMap<llvm::Function>& function_map, LLVMMap<Type>& type_map, LLVMMap<Value>& value_map, Module* module, IRBuilder<>* builder)
    {
        Buffer* name = node->proto->name;
        llvm::Function* search_result = function_map.find_value(name);
        redassert(!search_result);

        llvm::Function* function = module->getFunction(buf_ptr(name));
        if (!function)
        {
            function = fn_prototype(node->proto, type_map, module);
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
        builder->SetInsertPoint(bb);

        value_map.clear();
        auto args = function->args();
        for (const Argument& arg : args)
        {
            Buffer bf = {};
            buf_init_from_str(&bf, arg.getName().data());
            value_map.append(&bf, (Value*)&arg);
        }

        ReturnExpr* return_expr = static_cast<ReturnExpr*>(node->body);
        assert(return_expr);
        llvm::Value* return_value = fn_return_expr(return_expr);
        redassert(return_value);
        if (return_value)
        {
            ReturnInst* ret_inst = builder->CreateRet(return_value);
            bool errors = verifyFunction(*function, &llvm::errs());
            if (errors)
            {
                exit(1);
            }
            return function;
        }

        function->eraseFromParent();
        return nullptr;
    }

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
    ExplicitTimer llvm_ir_init("IniLL");
    context = new LLVMContext();
    Module* module = new Module("red_module", *context);
    IRBuilder<>* builder = new IRBuilder<>(*context);

    LLVMMap<llvm::Function> function_map = {};
    LLVMMap<Type> type_map = {};
    LLVMMap<Value> value_map = {};

    RedIR::register_types(type_map);
    
    llvm_ir_init.end();
    ExplicitTimer llvm_ir_generation("IRLLV");
    for (u32 i = 0; i < fn_list->length; i++)
    {
        RedAST::Function* ast_fn = fn_list->at(i);
        llvm::Function* ir_fn = RedIR::fn_definition(ast_fn, function_map, type_map, value_map, module, builder);
    }
    llvm_ir_generation.end();

    RedMachineCodeGen::output_obj_file(module);
}