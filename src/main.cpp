//
// Created by David on 04/10/2020.
//

#include "types.h"
#include "lexer.h"
#include "file.h"
#include "src_file.h"
#include "logger.h"
#include "os.h"
#include "config.h"

struct FileManager
{
    Buffer* buffers;
    char** filenames;
    size_t count;
};

static FileManager handle_main_arguments(s32 argc, char* argv[]);
static void FileManager_cleanup(FileManager* fm);

s32 main(s32 argc, char* argv[])
{
    FileManager fm = handle_main_arguments(argc, argv);
    if (fm.count == 0)
        return 0;

    for (size_t i = 0; i < fm.count; i++)
    {
        Buffer* src_buffer = &fm.buffers[i];
        add_source_file(src_buffer, argv[i + 1]);
    }

    FileManager_cleanup(&fm);
    return 0;
}

static inline void print_header(void)
{
    print("Red language compiler\n");
}


static FileManager handle_main_arguments(s32 argc, char* argv[])
{
    Buffer* cwd = buf_alloc();
    os_get_cwd(cwd);
#if CWD_VERBOSE
    print("CWD: %s\n", buf_ptr(cwd));
#endif
    FileManager fm = {0};
    FileLoadResult file_load_result;
    print_header();

    if (argc < 2)
    {
        print("Error: not enough arguments\n");
        print("\tUsage: -h, --help\n");
        return fm;
    }

    // TODO: check that file names are valid

    size_t file_count = argc - 1;
    fm.buffers = new_elements(Buffer, file_count);
    fm.filenames = new_elements(char*, file_count);
    if (fm.buffers)
    {
        fm.count = file_count;
        for (size_t i = 0; i < file_count; i++)
        {
            char* filename = argv[i + 1];
            fm.filenames[i] = filename;
            file_load_result = file_load(fm.filenames[i], &fm.buffers[i]);
            if (file_load_result != FILE_LOAD_RESULT_SUCCESS)
            {
                print("Failed to load file: %s\n", filename);
                FileManager_cleanup(&fm);
                return fm;
            }
        }
    }

    return fm;
}
static void FileManager_cleanup(FileManager* fm)
{
    if (fm)
    {
        if (fm->buffers)
        {
            free(fm->buffers);
        }
        if (fm->filenames)
        {
            free(fm->filenames);
        }
        memset(fm, 0, sizeof(FileManager));
    }
}
