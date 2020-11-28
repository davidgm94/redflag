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

static FileManager handle_main_arguments(s32 argc, char* argv[], bool cmdline_args);
static void FileManager_cleanup(FileManager* fm);

void print_memory_usage(void)
{
    u64 mem_usage = get_memory_usage();
    u64 block_size = get_block_size();
    u64 alloc_count = get_total_allocations();
    print("\nMemory usage: %llu bytes. Available: %llu bytes. Relative usage: %02.02f%%. Total allocations: %llu\n", mem_usage, block_size, ((f64)mem_usage / (f64)block_size) * 100.0f, alloc_count);
}
s32 main(s32 argc, char* argv[])
{
    os_init(mem_init);
    s64 start = os_performance_counter();

    ExplicitTimer file_dt = os_timer_start("File");
#if 0
    FileManager fm = handle_main_arguments(argc, argv);
#else
    char* src_files[] =
    {
#if RED_RUN_NOT_PASSING
        "not_passing.red",
#else
        "test.red", "first_hello_world.red",
#endif
    };
#if RED_CWD_VERBOSE
    SB* cwd = os_get_cwd();
    print("CWD: %s\n", sb_ptr(cwd));
#endif
    FileManager fm = handle_main_arguments(array_length(src_files), src_files, false);
#endif
    if (fm.count == 0)
    {
        os_exit_with_message("Can't load files\n");
    }

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
    print_memory_usage();

    return 0;
}

//static inline void print_header(void)
//{
//    print("Red language compiler\n");
//}

static FileManager handle_main_arguments(s32 argc, char* argv[], bool cmdline_args)
{
    //ExplicitTimer cwd_dt = et_start("cwd");
    //SB* cwd = os_get_cwd();
    //et_end(&cwd_dt);
#if RED_CWD_VERBOSE
    print("CWD: %s\n", buf_ptr(cwd));
#endif
    FileManager fm = {0};
    //print_header();

    if (cmdline_args && argc < 2)
    {
        print("Error: not enough arguments\n");
        print("\tUsage: -h, --help\n");
        return fm;
    }

    // TODO: check that file names are valid

    size_t file_count = argc - cmdline_args;
    fm.buffers = NEW(SB*, file_count);
    fm.filenames = NEW(char*, file_count);
    if (fm.buffers)
    {
        fm.count = file_count;
        for (usize i = 0; i < file_count; i++)
        {
            char* filename = argv[i + cmdline_args];
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

