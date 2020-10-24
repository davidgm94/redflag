#include "compiler_types.h"
#include "codegen.h"

#include "red_llvm.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DiagnosticInfo.h>
#include <llvm/IR/DiagnosticPrinter.h>

struct CodeGenErrorMessage
{
    s32 line_start;
    s32 column_start;
    s32 line_end;
    s32 column_end;
    Buffer* message;
};

enum CodeGenTypeID
{
    CODEGEN_TYPE_ID_USER_DEFINED,
    CODEGEN_TYPE_ID_POINTER,
    CODEGEN_TYPE_ID_U8,
    CODEGEN_TYPE_ID_S32,
    CODEGEN_TYPE_UNREACHABLE,
};

struct TypeTableEntry
{
    CodeGenTypeID id;
    LLVMTypeRef type_ref;
    llvm::DIType* di_type;

    TypeTableEntry* child_ptr;
    bool pointer_is_const;
    s32 user_defined_id;
    Buffer name;
    TypeTableEntry* const_ptr_parent;
    TypeTableEntry* var_ptr_parent;
};

struct FnTableEntry
{
    LLVMValueRef fn_value;
    ASTNode* proto_node;
    ASTNode* fn_def_node;
    bool is_extern;
    bool internal_linkage;
};

enum CodeGenBuildType
{
    CODEGEN_BUILD_TYPE_DEBUG,
    CODEGEN_BUILD_TYPE_RELEASE,
};

struct CodeGen
{
    LLVMModuleRef module;
    ASTNode* root;
    List<CodeGenErrorMessage> errors;
    LLVMBuilderRef builder;
    llvm::DIBuilder* dbuilder;
    llvm::DICompileUnit* compile_unit;
    HashMap<Buffer*, FnTableEntry*, buf_hash, buf_eql_buf> fn_table;
    HashMap<Buffer*, LLVMValueRef, buf_hash, buf_eql_buf> str_table;
    HashMap<Buffer*, TypeTableEntry, buf_hash, buf_eql_buf> type_table;
    HashMap<Buffer*, bool, buf_hash, buf_eql_buf> link_table;
    TypeTableEntry* invalid_type_entry;
    LLVMTargetDataRef target_data_ref;
    u32 pointer_size_bytes;
    bool is_static;
    bool strip_debug_symbols;
    CodeGenBuildType build_type;
    LLVMTargetMachineRef target_machine;
    Buffer input_file;
    Buffer input_directory;
    List<llvm::DIScope*> block_scopes;
    llvm::DIFile* di_file;
    List<FnTableEntry*> fn_definitions;
};

namespace codegen
{
    static inline void setup_config(CodeGen* g, CodeGenConfig* config)
    {
        g->build_type = config->release ? CODEGEN_BUILD_TYPE_RELEASE : CODEGEN_BUILD_TYPE_DEBUG;
        g->strip_debug_symbols = config->strip_debug_symbols;
        g->is_static = config->is_static;
    }

    static inline void add_types(CodeGen* g)
    {
        {
            TypeTableEntry* entry = NEW<TypeTableEntry>(1);
            entry->id = CODEGEN_TYPE_ID_U8;
            entry->type_ref = LLVMInt8Type();
            buf_init_from_str(&entry->name, "u8");
            entry->di_type = g->dbuilder->createBasicType(buf_ptr(&entry->name), 8, 8);
            g->type_table.put(&entry->name, *entry);

        }
        {
            TypeTableEntry* entry = NEW<TypeTableEntry>(1);
            entry->id = CODEGEN_TYPE_ID_S32;
            buf_init_from_str(&entry->name, "s32");
            entry->di_type = g->dbuilder->createBasicType(buf_ptr(&entry->name), 32, 32);
            g->type_table.put(&entry->name, *entry);
            g->invalid_type_entry = entry;
        }
        {
            TypeTableEntry* entry = NEW<TypeTableEntry>(1);
            entry->id = CODEGEN_TYPE_UNREACHABLE;
            entry->type_ref = LLVMVoidType();
            buf_init_from_str(&entry->name, "unreachable");
            entry->di_type = g->invalid_type_entry->di_type;
            g->type_table.put(&entry->name, *entry);
        }
    }

    static inline void add_node_error(CodeGen* g, ASTNode* node, Buffer* message)
    {
        g->errors.add_one();
        CodeGenErrorMessage* last_message = &g->errors.last();
        last_message->line_start = node->line;
        last_message->column_start = node->column;
        last_message->line_end = -1;
        last_message->column_end = -1;
        last_message->message = message;
    }

    static inline void resolve_type_and_recurse(CodeGen* g, ASTNode* node)
    {
        assert(!node->codegen_node);
        node->codegen_node = NEW<CodeGenNode>(1);
        TypeNode* type_node = &node->codegen_node->data.type_node;

        // TODO: CONSIDER PRIMITIVE VS PTR
        //switch (node->data.type.type)
        //{
        //    default:
        //        break;
        //}

        Buffer* name = node->data.type.type;
        auto table_entry = g->type_table.maybe_get(name);
        if (table_entry)
        {
            type_node->entry = &table_entry->value;
        }
        else
        {
            add_node_error(g, node, buf_sprintf("invalid type name: '%s'", buf_ptr(name)));
            type_node->entry = g->invalid_type_entry;
        }
    }

    static inline void find_declarations(CodeGen* g, ASTNode* node)
    {
        switch (node->type)
        {
            case NODE_TYPE_FN_DEF:
            {
                ASTNode* proto_node = node->data.fn_definition.function_prototype;
                assert(proto_node->type == NODE_TYPE_FN_PROTO);
                Buffer* proto_name = proto_node->data.fn_prototype.name;
                auto entry = g->fn_table.maybe_get(proto_name);
                if (entry)
                {
                    add_node_error(g, node, buf_sprintf("redefinition of '%s'", buf_ptr(proto_name)));
                }
                else
                {
                    FnTableEntry* fn_table_entry = NEW<FnTableEntry>(1);
                    fn_table_entry->proto_node = proto_node;
                    fn_table_entry->fn_def_node = node;
                    g->fn_table.put(proto_name, fn_table_entry);
                    g->fn_definitions.append(fn_table_entry);

                    find_declarations(g, proto_node);
                }
                break;
            }
            case NODE_TYPE_FN_PROTO:
            {
                for (usize i = 0; i < node->data.fn_prototype.parameters.length; i++)
                {
                    ASTNode* child = node->data.fn_prototype.parameters.at(i);
                    find_declarations(g, child);
                }
                break;
            }
            case NODE_TYPE_PARAM_DECL:
                find_declarations(g, node->data.param_decl.type);
                break;
            case NODE_TYPE_TYPE:
                resolve_type_and_recurse(g, node);
            default:
                RED_PANIC("Unexpected type in find_declarations(): %d\n", node->type);
                break;
        }
    }

    static inline void analyze_node(CodeGen* g, ASTNode* node)
    {
        switch (node->type)
        {
            case NODE_TYPE_CONTAINER_DECL:
            {
                if (node->data.container_decl.is_root)
                {
                    for (usize i = 0; i < node->data.container_decl.declarations.length; i++)
                    {
                        ASTNode* child = node->data.container_decl.declarations.at(i);
                        find_declarations(g, child);
                    }
                    for (usize i = 0; i < node->data.container_decl.declarations.length; i++)
                    {
                        ASTNode* child = node->data.container_decl.declarations.at(i);
                        analyze_node(g, child);
                    }
                }
                break;
            }
            case NODE_TYPE_FN_DEF:
            {
                ASTNode* proto_node = node->data.fn_definition.function_prototype;
                assert(proto_node->type == NODE_TYPE_FN_PROTO);
                analyze_node(g, proto_node);
                break;
            }
            case NODE_TYPE_FN_PROTO:
            {
                for (usize i = 0; i < node->data.fn_prototype.parameters.length; i++)
                {
                    ASTNode* child = node->data.fn_prototype.parameters.at(i);
                    analyze_node(g, child);
                }

                analyze_node(g, node->data.fn_prototype.return_type);
                break;
            }
            case NODE_TYPE_PARAM_DECL:
            {
                analyze_node(g, node->data.param_decl.type);
                break;
            }
            case NODE_TYPE_TYPE:
                // find_declarations
                break;
            case NODE_TYPE_FN_CALL_EXPR:
            {
                // TODO: fix this
                Buffer* name = node->data.fn_call_expr.function->data.fn_definition.function_prototype->data.fn_prototype.name;

                auto entry = g->fn_table.maybe_get(name);
                if (!entry)
                {
                    add_node_error(g, node, buf_sprintf("undefined function: '%s'", buf_ptr(name)));
                }
                else
                {
                    FnTableEntry* fn_table_entry = entry->value;
                    assert(fn_table_entry->proto_node->type == NODE_TYPE_FN_PROTO);
                    s32 expected_param_count = fn_table_entry->proto_node->data.fn_prototype.parameters.length;
                    s32 actual_param_count = fn_table_entry->proto_node->data.fn_call_expr.parameters.length;

                    if (expected_param_count != actual_param_count)
                    {
                        add_node_error(g, node, buf_sprintf("wrong number of arguments. Expected %d, got %d.",
                            expected_param_count, actual_param_count));
                    }
                }

                for (usize i = 0; i < node->data.fn_call_expr.parameters.length; i++)
                {
                    ASTNode* child = node->data.fn_call_expr.parameters.at(i);
                    analyze_node(g, child);
                }

                break;
            }
            default:
                RED_PANIC("Unexpected node type: %d\n", node->type);
                break;
        }
    }

    static inline void semantic_analyze(CodeGen* g)
    {
        LLVMInitializeAllTargets();
        LLVMInitializeAllTargetMCs();
        LLVMInitializeAllAsmPrinters();
        LLVMInitializeAllAsmParsers();
        LLVMInitializeNativeTarget();

        char* native_triple = LLVMGetDefaultTargetTriple();

        LLVMTargetRef target_ref;
        char* error_message = nullptr;

        if (LLVMGetTargetFromTriple(native_triple, &target_ref, &error_message))
        {
            RED_PANIC("Unable to get target from triple: %s", error_message);
        }

        char* native_cpu = red_llvm::get_host_CPU_name();
        char* native_features = red_llvm::get_native_features();

        LLVMCodeGenOptLevel opt_level = (g->build_type == CODEGEN_BUILD_TYPE_DEBUG) ? LLVMCodeGenLevelNone : LLVMCodeGenLevelAggressive;

        LLVMRelocMode reloc_mode = g->is_static ? LLVMRelocStatic : LLVMRelocPIC;

        g->target_machine = LLVMCreateTargetMachine(target_ref, native_triple, native_cpu, native_features, opt_level, reloc_mode, LLVMCodeModelDefault);
        g->target_data_ref = LLVMCreateTargetDataLayout(g->target_machine);

        g->module = LLVMModuleCreateWithName("RedModule");
        g->pointer_size_bytes = LLVMPointerSize(g->target_data_ref);
        g->builder = LLVMCreateBuilder();
        g->dbuilder = new llvm::DIBuilder(*llvm::unwrap(g->module), true);

        add_types(g);
        
        analyze_node(g, g->root);
    }

    static inline LLVMValueRef gen_expr(CodeGen* g, ASTNode* expr_node)
    {
        RED_NOT_IMPLEMENTED;
        return nullptr;
    }

    static inline LLVMTypeRef to_llvm_type(ASTNode* type_node)
    {
        assert(type_node->type == NODE_TYPE_TYPE);
        assert(type_node->codegen_node);
        assert(type_node->codegen_node->data.type_node.entry);

        return type_node->codegen_node->data.type_node.entry->type_ref;
    }

    static inline llvm::DIType* to_llvm_debug_type(ASTNode* type_node)
    {
        assert(type_node->type == NODE_TYPE_TYPE);
        assert(type_node->codegen_node);
        assert(type_node->codegen_node->data.type_node.entry);

        return type_node->codegen_node->data.type_node.entry->di_type;
    }

    static inline llvm::DISubroutineType* create_di_function_type(CodeGen* g, ASTNodeFunctionPrototype* fn_proto, llvm::DIFile* di_file)
    {
        llvm::SmallVector<llvm::Metadata*, 8> types;

        llvm::DIType* return_type = to_llvm_debug_type(fn_proto->return_type);
        types.push_back(return_type);

        for (s32 i = 0; i < fn_proto->parameters.length; i++)
        {
            ASTNode* param_node = fn_proto->parameters.at(i);
            llvm::DIType* param_type = to_llvm_debug_type(param_node);
            types.push_back(param_type);
        }

        return g->dbuilder->createSubroutineType(g->dbuilder->getOrCreateTypeArray(types));
    }

    static inline void add_debug_source_node(CodeGen* g, ASTNode* node)
    {
        llvm::unwrap(g->builder)->SetCurrentDebugLocation(
            llvm::DebugLoc::get(node->line + 1, node->column + 1, g->block_scopes.last())
            );
    }

    static inline void gen_block(CodeGen* g, ASTNode* block_node)
    {
        assert(block_node->type == NODE_TYPE_BLOCK);

        llvm::DILexicalBlock* di_block = g->dbuilder->createLexicalBlock(g->block_scopes.last(),
            g->di_file, block_node->line + 1, block_node->column + 1);
        g->block_scopes.append(di_block);

        for (s32 i = 0; i < block_node->data.block.statements.length; i++)
        {
            ASTNode* statement_node = block_node->data.block.statements.at(i);

            switch (statement_node->type)
            {
                case NODE_TYPE_RETURN_EXPR:
                {
                    ASTNode* expr_node = statement_node->data.return_expr.expression;
                    LLVMValueRef value = gen_expr(g, expr_node);
                    
                    add_debug_source_node(g, statement_node);
                    LLVMBuildRet(g->builder, value);
                    break;
                }

                case NODE_TYPE_BIN_OP_EXPR:
                {
                    gen_expr(g, statement_node);
                    break;
                }
                default:
                    RED_UNREACHABLE;
            }
        }

        g->block_scopes.pop();
    }

    static inline void gen_machine_code(CodeGen* g)
    {
        assert(!g->errors.length);

        Buffer* producer = buf_sprintf("red %s", RED_VERSION_STRING);
        bool is_optimized = g->build_type == CODEGEN_BUILD_TYPE_RELEASE;
        const char* flags = "";
        u32 runtime_version = 0;
        llvm::DIFile* input_file = g->dbuilder->createFile(g->input_file.items, g->input_directory.items);
        g->compile_unit = g->dbuilder->createCompileUnit(llvm::dwarf::DW_LANG_C99, input_file,
            producer->items, is_optimized, flags, runtime_version);
        g->block_scopes.append(g->compile_unit);
        //g->di_file = 

        auto it = g->fn_table.entry_iterator();
        for (;;)
        {
            auto* entry = it.next();
            if (!entry)
            {
                break;
            }

            FnTableEntry* fn_table_entry = entry->value;

            ASTNode* proto_node = fn_table_entry->proto_node;
            assert(proto_node->type == NODE_TYPE_FN_PROTO);
            ASTNodeFunctionPrototype* fn_proto = &proto_node->data.fn_prototype;

            LLVMTypeRef return_type = to_llvm_type(fn_proto->return_type);
            LLVMTypeRef* parameter_types = NEW<LLVMTypeRef>(fn_proto->parameters.length);

            for (usize i = 0; i < fn_proto->parameters.length; i++)
            {
                ASTNode* param_node = fn_proto->parameters.at(i);
                assert(param_node->type == NODE_TYPE_PARAM_DECL);
                ASTNode* type_node = param_node->data.param_decl.type;
                parameter_types[i] = to_llvm_type(type_node);
            }

            LLVMTypeRef function_type = LLVMFunctionType(return_type, parameter_types, fn_proto->parameters.length, false);
            LLVMValueRef fn = LLVMAddFunction(g->module, buf_ptr(fn_proto->name), function_type);

            LLVMSetLinkage(fn, fn_table_entry->internal_linkage ? LLVMPrivateLinkage : LLVMExternalLinkage);

            LLVMSetFunctionCallConv(fn, LLVMCCallConv);

            fn_table_entry->fn_value = fn;
        }

        for (usize i = 0; i < g->fn_definitions.length; i++)
        {
            FnTableEntry* fn_table_entry = g->fn_definitions.at(i);
            ASTNode* fn_def_node = fn_table_entry->fn_def_node;
            LLVMValueRef fn = fn_table_entry->fn_value;

            ASTNode* proto_node = fn_table_entry->proto_node;
            assert(proto_node->type == NODE_TYPE_FN_PROTO);
            ASTNodeFunctionPrototype* fn_proto = &proto_node->data.fn_prototype;

            llvm::DIScope* fn_scope = g->di_file;
            u32 line_number = fn_def_node->line + 1;
            u32 scope_line = line_number;
            bool is_definition = true;
            u32 flags = 0;
            llvm::Function* unwrapped_function = reinterpret_cast<llvm::Function*>(llvm::unwrap(fn));
            llvm::DISubprogram* subprogram = g->dbuilder->createFunction(fn_scope,
                buf_ptr(fn_proto->name), "", g->di_file, line_number, create_di_function_type(g, fn_proto, g->di_file),
                scope_line);

            g->block_scopes.append(subprogram);

            LLVMBasicBlockRef entry_block = LLVMAppendBasicBlock(fn, "entry");
            LLVMPositionBuilderAtEnd(g->builder, entry_block);

            gen_block(g, fn_def_node->data.fn_definition.body);

            g->block_scopes.pop();
        }

        assert(!g->errors.length);

        g->dbuilder->finalize();
        LLVMDumpModule(g->module);

        char* error = nullptr;
        LLVMVerifyModule(g->module, LLVMAbortProcessAction, &error);
    }

    static inline void link(CodeGen* g, const char* out_file)
    {
        LLVMPassRegistryRef registry = LLVMGetGlobalPassRegistry();
        LLVMInitializeCore(registry);
        LLVMInitializeCodeGen(registry);
        red_llvm::initialize_loop_strength_reduce_pass(registry);
        red_llvm::initialize_lower_intrinsics_pass(registry);
        red_llvm::initialize_unreachable_block_elim_pass(registry);

        Buffer outfile_o = {};
        buf_init_from_str(&outfile_o, out_file);
        buf_append_str(&outfile_o, ".o");

        char* error_message = nullptr;
        if (LLVMTargetMachineEmitToFile(g->target_machine, g->module, buf_ptr(&outfile_o), LLVMObjectFile, &error_message))
        {
            RED_PANIC("Unable to write obj file %s!", error_message);
        }

        List<const char*> arguments = { };
        
        if (g->is_static)
        {
            arguments.append("-static");
        }

        arguments.append("-o");
        arguments.append(out_file);
        arguments.append((const char*)buf_ptr(&outfile_o));

        auto it = g->link_table.entry_iterator();
        for (;;)
        {
            auto* entry = it.next();
            if (!entry)
            {
                break;
            }

            Buffer* argument = buf_sprintf("-l%s", buf_ptr(entry->key));
            arguments.append(buf_ptr(argument));
        }
        
        // Implement linker call
        RED_NOT_IMPLEMENTED;
    }
}

void red_codegen_and_link(ASTNode* root_node, CodeGenConfig* config, Buffer* input_file, Buffer* output_file)
{
    CodeGen* g = NEW<CodeGen>(1);
    memset(g, 0, sizeof(CodeGen));
    g->root = root_node;
    g->fn_table.init(32);
    g->str_table.init(32);
    g->type_table.init(32);
    g->link_table.init(32);
    codegen::setup_config(g, config);

    codegen::semantic_analyze(g);
    List<CodeGenErrorMessage>* errors = &g->errors;
    if (errors->length == 0)
    {
        fprintf(stdout, "OK\n");
    }
    else
    {
        for (usize i = 0; i < errors->length; i++)
        {
            CodeGenErrorMessage* error = &errors->at(i);
            fprintf(stdout, "Error in line %d, column %d: %s\n", error->line_start, error->column_start, buf_ptr(error->message));
        }

        exit(1);
    }

    codegen::gen_machine_code(g);

    codegen::link(g, buf_ptr(output_file));
}
