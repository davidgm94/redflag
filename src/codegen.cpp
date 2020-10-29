#include "compiler_types.h"
#include "codegen.h"

#include "red_llvm.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DiagnosticInfo.h>
#include <llvm/IR/DiagnosticPrinter.h>
#include "os.h"

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
    CODEGEN_TYPE_VOID,
    CODEGEN_TYPE_STRING,
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
    Buffer* input_file;
    Buffer* input_directory;
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

    static inline bool type_unreachable(ASTNode* type_node)
    {
        redassert(type_node->type == NODE_TYPE_TYPE);
        redassert(type_node->codegen_node);
        redassert(type_node->codegen_node->type_node.entry);
        return type_node->codegen_node->type_node.entry->id == CODEGEN_TYPE_UNREACHABLE;
    }

    static inline void add_types(CodeGen* g)
    {
        TypeTableEntry* u8_entry;
        {
            TypeTableEntry* entry = NEW<TypeTableEntry>(1);
            entry->id = CODEGEN_TYPE_ID_U8;
            entry->type_ref = LLVMInt8Type();
            buf_init_from_str(&entry->name, "u8");
            entry->di_type = g->dbuilder->createBasicType(buf_ptr(&entry->name), 8, 8);
            g->type_table.put(&entry->name, *entry);
            u8_entry = entry;
        }
        {
            TypeTableEntry* entry = NEW<TypeTableEntry>(1);
            entry->id = CODEGEN_TYPE_ID_S32;
            entry->type_ref = LLVMInt32Type();
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
        {
            TypeTableEntry* entry = NEW<TypeTableEntry>(1);
            entry->id = CODEGEN_TYPE_VOID;
            entry->type_ref = LLVMVoidType();
            buf_init_from_str(&entry->name, "void");
            entry->di_type = g->invalid_type_entry->di_type;
            g->type_table.put(&entry->name, *entry);
        }
        {
            TypeTableEntry* entry = NEW<TypeTableEntry>(1);
            entry->id = CODEGEN_TYPE_STRING;
            entry->type_ref = LLVMPointerType(LLVMInt8Type(), 0);
            buf_init_from_str(&entry->name, "String");
            entry->di_type = g->dbuilder->createPointerType(u8_entry->di_type, 64, g->pointer_size_bytes * 8, g->pointer_size_bytes * 8, buf_ptr(&entry->name));
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
        redassert(!node->codegen_node);
        node->codegen_node = NEW<CodeGenNode>(1);
        TypeNode* type_node = &node->codegen_node->type_node;

        // TODO: CONSIDER PRIMITIVE VS PTR
        //switch (node->type.type)
        //{
        //    default:
        //        break;
        //}

        Buffer* name = node->type_expr.type;
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
                //case NODE_TYPE_CONTAINER_DECL:
                //{
                //    ASTNodeContainerDeclaration* container_decl = &node->container_decl;
                //    bool is_root = container_decl->is_root;
                //    if (is_root)
                //    {
                //        List<ASTNode*> declarations = container_decl->declarations;
                //        for (u32 i = 0; i < declarations.length; i++)
                //        {
                //            find_declarations(g, declarations.at(i));
                //        }
                //    }
                //    else
                //    {
                //        List<ASTNode*> fields = container_decl->fields;
                //        for (usize i = 0; i < fields.length; i++)
                //        {
                //            find_declarations(g, fields.at(i));
                //        }
                //    }
                //    break;
                //}
                case NODE_TYPE_FN_DEF:
                {
                    ASTNode* proto_node = node->fn_definition.function_prototype;
                    redassert(proto_node->type == NODE_TYPE_FN_PROTO);
                    Buffer* proto_name = proto_node->fn_prototype.name;
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
                //case NODE_TYPE_VARIABLE_DECLARATION:
                //{
                //    auto* var_decl = &node->variable_declaration;
                //    find_declarations(g, var_decl->type);
                //    break;
                //}
                case NODE_TYPE_FN_PROTO:
                {
                    if (node->fn_prototype.external_linkage)
                    {
                        for (usize i = 0; i < node->fn_prototype.parameters.length; i++)
                        {
                            ASTNode* child = node->fn_prototype.parameters.at(i);
                            find_declarations(g, child);
                        }
                        Buffer* name = node->fn_prototype.name;
                        FnTableEntry* fn_table_entry = NEW<FnTableEntry>(1);
                        fn_table_entry->proto_node = node;
                        fn_table_entry->is_extern = true;
                        g->fn_table.put(name, fn_table_entry);
                    }
                    else
                    {
                        for (usize i = 0; i < node->fn_prototype.parameters.length; i++)
                        {
                            ASTNode* child = node->fn_prototype.parameters.at(i);
                            find_declarations(g, child);
                        }
                    }
                    break;
                }
                case NODE_TYPE_PARAM_DECL:
                    find_declarations(g, node->param_decl.type);
                    break;
                case NODE_TYPE_TYPE:
                    resolve_type_and_recurse(g, node);
                    break;
                default:
                    //RED_PANIC("Unexpected type in find_declarations(): %d\n", node->type);
                    break;
            }
    }

    static inline void analyze_node(CodeGen* g, ASTNode* node)
    {
        switch (node->type)
        {
            case NODE_TYPE_CONTAINER_DECL:
            {
                if (node->container_decl.is_root)
                {
                    for (usize i = 0; i < node->container_decl.declarations.length; i++)
                    {
                        ASTNode* child = node->container_decl.declarations.at(i);
                        find_declarations(g, child);
                    }
                    for (usize i = 0; i < node->container_decl.declarations.length; i++)
                    {
                        ASTNode* child = node->container_decl.declarations.at(i);
                        analyze_node(g, child);
                    }
                }
                break;
            }
            case NODE_TYPE_FN_DEF:
            {
                ASTNode* proto_node = node->fn_definition.function_prototype;
                redassert(proto_node->type == NODE_TYPE_FN_PROTO);
                analyze_node(g, proto_node);
                break;
            }
            case NODE_TYPE_FN_PROTO:
            {
                for (usize i = 0; i < node->fn_prototype.parameters.length; i++)
                {
                    ASTNode* child = node->fn_prototype.parameters.at(i);
                    analyze_node(g, child);
                }

                find_declarations(g, node->fn_prototype.return_type);
                analyze_node(g, node->fn_prototype.return_type);
                break;
            }
            case NODE_TYPE_PARAM_DECL:
            {
                analyze_node(g, node->param_decl.type);
                break;
            }
            case NODE_TYPE_TYPE:
                // find_declarations
                break;
            case NODE_TYPE_FN_CALL_EXPR:
            {
                Buffer* name = node->fn_call_expr.name;

                auto entry = g->fn_table.maybe_get(name);
                if (!entry)
                {
                    add_node_error(g, node, buf_sprintf("undefined function: '%s'", buf_ptr(name)));
                }
                else
                {
                    FnTableEntry* fn_table_entry = entry->value;
                    redassert(fn_table_entry->proto_node->type == NODE_TYPE_FN_PROTO);
                    s32 expected_param_count = fn_table_entry->proto_node->fn_prototype.parameters.length;
                    s32 actual_param_count = fn_table_entry->proto_node->fn_call_expr.parameters.length;

                    if (expected_param_count != actual_param_count)
                    {
                        add_node_error(g, node, buf_sprintf("wrong number of arguments. Expected %d, got %d.",
                            expected_param_count, actual_param_count));
                    }
                }

                for (usize i = 0; i < node->fn_call_expr.parameters.length; i++)
                {
                    ASTNode* child = node->fn_call_expr.parameters.at(i);
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

    static inline LLVMValueRef find_or_create_string(CodeGen* g, Buffer* str)
    {
        auto entry = g->str_table.maybe_get(str);
        if (entry)
        {
            return entry->value;
        }

        LLVMValueRef text = LLVMConstString(buf_ptr(str), buf_len(str), false);
        LLVMValueRef global_value = LLVMAddGlobal(g->module, LLVMTypeOf(text), "");
        LLVMSetLinkage(global_value, LLVMPrivateLinkage);
        LLVMSetInitializer(global_value, text);
        LLVMSetGlobalConstant(global_value, true);
        LLVMSetUnnamedAddr(global_value, true);
        g->str_table.put(str, global_value);

        return global_value;
    }

    static inline LLVMValueRef gen_expr(CodeGen* g, ASTNode* node)
    {
        switch (node->type)
        {
            case NODE_TYPE_STRING_LITERAL:
            {
                Buffer* string_value = node->string_literal.buffer;
                LLVMValueRef llvm_str_value = find_or_create_string(g, string_value);
                LLVMValueRef indices[] =
                {
                    LLVMConstInt(LLVMInt32Type(), 0, false),
                    LLVMConstInt(LLVMInt32Type(), 0, false),
                };
                LLVMValueRef ptr_value = LLVMBuildInBoundsGEP(g->builder, llvm_str_value, indices, 2, "");

                return ptr_value;
            }
            case NODE_TYPE_INT_LITERAL:
            {
                // TODO: change for more than 1 digit
                LLVMValueRef value = LLVMConstInt(LLVMInt32Type(), node->int_literal.big_int->digit, node->int_literal.big_int->is_negative);
                return value;
            }
            default:
                RED_NOT_IMPLEMENTED;
                break;
        }
        return nullptr;
    }

    static inline LLVMTypeRef to_llvm_type(ASTNode* type_node)
    {
        redassert(type_node->type == NODE_TYPE_TYPE);
        redassert(type_node->codegen_node);
        redassert(type_node->codegen_node->type_node.entry);

        return type_node->codegen_node->type_node.entry->type_ref;
    }

    static inline llvm::DIType* to_llvm_debug_type(ASTNode* type_node)
    {
        redassert(type_node->type == NODE_TYPE_TYPE);
        redassert(type_node->codegen_node);
        redassert(type_node->codegen_node->type_node.entry);

        return type_node->codegen_node->type_node.entry->di_type;
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
        redassert(block_node->type == NODE_TYPE_BLOCK);
        llvm::DILexicalBlock* di_block = g->dbuilder->createLexicalBlock(g->block_scopes.last(),
            g->di_file, block_node->line + 1, block_node->column + 1);
        g->block_scopes.append(di_block);

        for (s32 i = 0; i < block_node->block.statements.length; i++)
        {
            ASTNode* node = block_node->block.statements.at(i);

            switch (node->type)
            {
                case NODE_TYPE_RETURN_EXPR:
                {
                    ASTNode* expr_node = node->return_expr.expression;
                    LLVMValueRef value = gen_expr(g, expr_node);
                    
                    add_debug_source_node(g, node);
                    LLVMBuildRet(g->builder, value);
                    break;
                }

                case NODE_TYPE_BIN_OP_EXPR:
                {
                    gen_expr(g, node);
                    break;
                }
                case NODE_TYPE_FN_CALL_EXPR:
                {
                    Buffer* name = node->fn_call_expr.name;
                    FnTableEntry* fn_table_entry = g->fn_table.get(name);
                    redassert(fn_table_entry->proto_node->type == NODE_TYPE_FN_PROTO);
                    usize expected_param_count = fn_table_entry->proto_node->fn_prototype.parameters.length;
                    usize actual_param_count = node->fn_call_expr.parameters.length;
                    redassert(expected_param_count == actual_param_count);

                    LLVMValueRef* param_values = NEW<LLVMValueRef>(actual_param_count);
                    for (usize i = 0; i < actual_param_count; i++)
                    {
                        ASTNode* expr_node = node->fn_call_expr.parameters.at(i);
                        param_values[i] = gen_expr(g, expr_node);
                    }

                    add_debug_source_node(g, node);

                    LLVMValueRef result = LLVMBuildCall(g->builder, fn_table_entry->fn_value, param_values, actual_param_count, "");

                    if (type_unreachable(fn_table_entry->proto_node->fn_prototype.return_type))
                    {
                        result = LLVMBuildUnreachable(g->builder);
                    }
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
        redassert(!g->errors.length);

        Buffer* producer = buf_sprintf("red %s", RED_VERSION_STRING);
        bool is_optimized = g->build_type == CODEGEN_BUILD_TYPE_RELEASE;
        const char* flags = "";
        u32 runtime_version = 0;
        llvm::DIFile* input_file = g->dbuilder->createFile(buf_ptr(g->input_file), buf_ptr(g->input_directory));
        g->compile_unit = g->dbuilder->createCompileUnit(llvm::dwarf::DW_LANG_C99, input_file,
            producer->items, is_optimized, flags, runtime_version);
        g->block_scopes.append(g->compile_unit);
        g->di_file = g->dbuilder->createFile(g->compile_unit->getFilename(), g->compile_unit->getDirectory());

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
            redassert(proto_node->type == NODE_TYPE_FN_PROTO);
            ASTNodeFunctionPrototype* fn_proto = &proto_node->fn_prototype;

            LLVMTypeRef return_type = to_llvm_type(fn_proto->return_type);
            LLVMTypeRef* parameter_types = NEW<LLVMTypeRef>(fn_proto->parameters.length);

            for (usize i = 0; i < fn_proto->parameters.length; i++)
            {
                ASTNode* param_node = fn_proto->parameters.at(i);
                redassert(param_node->type == NODE_TYPE_PARAM_DECL);
                ASTNode* type_node = param_node->param_decl.type;
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
            redassert(proto_node->type == NODE_TYPE_FN_PROTO);
            ASTNodeFunctionPrototype* fn_proto = &proto_node->fn_prototype;

            llvm::DIScope* fn_scope = g->di_file;
            u32 line_number = fn_def_node->line + 1;
            u32 scope_line = line_number;
            // @unused
            //bool is_definition = true;
            // @unused
            // u32 flags = 0;
            // @unused
            //llvm::Function* unwrapped_function = reinterpret_cast<llvm::Function*>(llvm::unwrap(fn));
            llvm::DISubprogram* subprogram = g->dbuilder->createFunction(fn_scope,
                buf_ptr(fn_proto->name), "", g->di_file, line_number, create_di_function_type(g, fn_proto, g->di_file),
                scope_line);

            g->block_scopes.append(subprogram);

            // TODO: this is a bloated mess
            LLVMBasicBlockRef entry_block = LLVMAppendBasicBlock(fn, "entry");
            LLVMPositionBuilderAtEnd(g->builder, entry_block);
            

            gen_block(g, fn_def_node->fn_definition.body);

            g->block_scopes.pop();
        }

        redassert(!g->errors.length);

        g->dbuilder->finalize();
        LLVMDumpModule(g->module);

        char* error = nullptr;
        if (LLVMVerifyModule(g->module, LLVMReturnStatusAction, &error))
        {
            if (error && *error)
            {
                print("LLVM Report: error: %s\n", error);
                exit(-1);
            }
        }
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
        
        //if (g->is_static)
        //{
        //    arguments.append("-static");
        //}

        //arguments.append("-o");
        //arguments.append(out_file);
        //arguments.append((const char*)buf_ptr(&outfile_o));
        arguments.append(buf_ptr(&outfile_o));
        arguments.append("/SUBSYSTEM:CONSOLE");

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
        
        const char* linker_executable = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Enterprise\\VC\\Tools\\MSVC\\14.27.29110\\bin\\Hostx64\\x64\\link.exe";
        Termination termination;
        print("Linker args: ");
        for (usize i = 0; i < arguments.length; i++)
        {
            print("%s ", arguments.at(i));
        }
        os_spawn_process(linker_executable, arguments, &termination);
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

    g->input_directory = buf_alloc();
    buf_init_from_str(g->input_directory, "C:/Users/david/git/redflag");
    g->input_file = buf_alloc();
    buf_init_from_str(g->input_file, "test_hello.red");

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
