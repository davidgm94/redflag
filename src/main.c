//
// Created by David on 04/10/2020.
//

#include "compiler_types.h"
#include "os.h"
#if RED_SELF_BACKEND
#include "x64_backend.h"
#else
#include "src_file.h"
#endif
typedef struct FileManager
{
    SB** buffers;
    char** filenames;
    size_t count;
} FileManager;

static FileManager handle_main_arguments(s32 argc, char* argv[]);
static void FileManager_cleanup(FileManager* fm);

#if RED_TIMESTAMPS
#endif

#include "llvm_codegen.h"

void verify_function(LLVMValueRef fn, const char* fn_type)
{
    if (LLVMVerifyFunction(fn, LLVMPrintMessageAction))
    {
        print("Function %s not verified\n", fn_type);
        os_exit(1);
    }
    else
    {
        print("Verified function %s\n", fn_type);
    }
}

void verify_module(LLVMModuleRef module)
{
    char* error = NULL;
    if (LLVMVerifyModule(module, LLVMPrintMessageAction, &error))
    {
        print("Module verified: %s\n", error);
        os_exit(1);
    }
    else
    {
        print("Verified module\n\n");
        print("%s\n", LLVMPrintModuleToString(module));
    }
    
}
s32 main(s32 argc, char* argv[])
{
#if 0
#if 0

    LLVMSetup llvm_cfg = llvm_setup(NULL);
    LLVMContextRef context = llvm_cfg.context;
    LLVMModuleRef module = llvm_setup_module("red_module", "test.red", &llvm_cfg);
    LLVMTypeRef int32_type = LLVMInt32TypeInContext(context);
    //LLVMTypeRef arg_types[] = { int32_type };
    //usize arg_count = array_length(arg_types);
    LLVMTypeRef* arg_types = NULL;
    usize arg_count = 0;
    LLVMTypeRef fn_type = LLVMFunctionType(int32_type, arg_types, arg_count, false);
    LLVMValueRef fn = LLVMAddFunction(module, "foo", fn_type);

    verify_function(fn, "prototype");

    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(context, fn, "entry");
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);
    LLVMPositionBuilderAtEnd(builder, entry);

    LLVMValueRef params[arg_count];
    LLVMValueRef params_allocs[arg_count];
    LLVMGetParams(fn, params);
    for (s32 i = 0; i < arg_count; i++)
    {
        LLVMSetValueName(params[i], "f1");
        params_allocs[i] = LLVMBuildAlloca(builder, LLVMTypeOf(params[i]), "");
    }

    if (arg_count > 0)
    {
        for (usize i = 0; i < arg_count; i++)
        {
            LLVMBuildStore(builder, params[i], params_allocs[i]);
        }
    }

    LLVMValueRef ret_value = LLVMConstInt(int32_type, 0, true);
    LLVMBuildRet(builder, ret_value);

    // LLVMPositionBuilderAtEnd(builder, previous_block);

    verify_function(fn, "definition");

    verify_module(module);


    
    
#else
    {
        s32 value = 5;
        get_constant_s32* make_const = make_constant_s32(5);
        s32 value_test = make_const(5);
        ptest("make_const", value == value_test);
    }

    {
        // TODO: identity_s64 takes no arguments: fix
        identity_s64* id_s64 = make_identity_s64();
        s64 result = id_s64(-158901);
        ptest("make_identity_s64", result == -158901);
    }

    {
        increment_s64* inc_s64 = make_increment_s64();
        s64 inc_result = inc_s64(1);
        ptest("inc_s64", inc_result == 2);
    }
#endif

#else
    os_init(mem_init);
    s64 start = os_performance_counter();

    ExplicitTimer file_dt = os_timer_start("File");
    FileManager fm = handle_main_arguments(argc, argv);
    if (fm.count == 0)
        return 0;

    os_timer_end(&file_dt);

    for (usize i = 0; i < fm.count; i++)
    {
        SB* src_buffer = fm.buffers[i];
        add_source_file(src_buffer, argv[i + 1]);
    }

    FileManager_cleanup(&fm);
    s64 end = os_performance_counter();
    f64 total_ms = os_compute_ms(start, end);
    os_print_recorded_times(total_ms);

    return 0;
#endif
}

//static inline void print_header(void)
//{
//    print("Red language compiler\n");
//}

static FileManager handle_main_arguments(s32 argc, char* argv[])
{
    //ExplicitTimer cwd_dt = et_start("cwd");
    //SB* cwd = os_get_cwd();
    //et_end(&cwd_dt);
#if RED_CWD_VERBOSE
    print("CWD: %s\n", buf_ptr(cwd));
#endif
    FileManager fm = {0};
    //print_header();

    if (argc < 2)
    {
        print("Error: not enough arguments\n");
        print("\tUsage: -h, --help\n");
        return fm;
    }

    // TODO: check that file names are valid

    size_t file_count = argc - 1;
    fm.buffers = NEW(SB*, file_count);
    fm.filenames = NEW(char*, file_count);
    if (fm.buffers)
    {
        fm.count = file_count;
        for (usize i = 0; i < file_count; i++)
        {
            char* filename = argv[i + 1];
            fm.filenames[i] = filename;
            fm.buffers[i] = os_file_load(fm.filenames[i]);
            if (!fm.buffers[i])
            {
                print("Failed to load file: %s\n", filename);
                FileManager_cleanup(&fm);
                fm.count = 0;
                return fm;
            }
        }
    }

    return fm;
}
static void FileManager_cleanup(FileManager* fm)
{
    //if (fm)
    //{
    //    if (fm->buffers)
    //    {
    //        free(fm->buffers);
    //    }
    //    if (fm->filenames)
    //    {
    //        free(fm->filenames);
    //    }
    //    memset(fm, 0, sizeof(FileManager));
    //}
}

