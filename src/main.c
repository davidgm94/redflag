//
// Created by David on 04/10/2020.
//

#include "compiler_types.h"
#include "os.h"
#include "src_file.h"

typedef struct File
{
    SB* file_buffer;
    char* filename;
} File;

static inline void print_memory_usage(void);
static inline void print_header(void);
static File handle_main_arguments(s32 argc, char* argv[]);

s32 main(s32 argc, char* argv[])
{
    os_init(mem_init);
    print_header();
    s64 start = os_performance_counter();

    ExplicitTimer file_dt = os_timer_start("File");
    File file = handle_main_arguments(argc, argv);
    os_timer_end(&file_dt);

    add_source_file(file.file_buffer, file.filename);

    s64 end = os_performance_counter();
    f64 total_ms = os_compute_ms(start, end);
    os_print_recorded_times(total_ms);
    print_memory_usage();

    return 0;
}

static inline void print_header(void)
{
    print("Red language compiler\n");
}

static File handle_main_arguments(s32 argc, char* argv[])
{
    //ExplicitTimer cwd_dt = et_start("cwd");
    //SB* cwd = os_get_cwd();
    //et_end(&cwd_dt);
#if RED_CWD_VERBOSE
    print("CWD: %s\n", buf_ptr(cwd));
#endif

    File file = ZERO_INIT;
    //print_header();

    if (argc < 2)
    {
        print("Error: not enough arguments\n");
        print("\tUsage: -h, --help\n");
        return file;
    }

    // TODO: check that file names are valid
    file.filename = argv[1];
    file.file_buffer = os_file_load(file.filename);
    if (!file.file_buffer)
    {
        os_exit_with_message("Failed to load file: %s\n", file.filename);
        return file; // @unreachable
    }

    return file;
}

static void print_memory_usage(void)
{
    u64 mem_usage = get_memory_usage();
    u64 block_size = get_block_size();
    u64 alloc_count = get_total_allocations();
    print("\nMemory usage: %llu bytes. Available: %llu bytes. Relative usage: %02.02f%%. Total allocations: %llu\n", mem_usage, block_size, ((f64)mem_usage / (f64)block_size) * 100.0f, alloc_count);
}
