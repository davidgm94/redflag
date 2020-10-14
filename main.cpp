//
// Created by David on 04/10/2020.
//

#include "types.h"
#include "lexer.h"
#include "file.h"
#include "src_file.h"

struct FileManager
{
    Buffer* buffers;
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
        add_source_file(nullptr, nullptr, nullptr, src_buffer, PKG_MAIN);
    }

    FileManager_cleanup(&fm);
    return 0;
}

static inline void print_header(void)
{
    printf("Red language compiler\n");
}


static FileManager handle_main_arguments(s32 argc, char* argv[])
{
    FileManager fm = {0};
    FileLoadResult file_load_result;
    print_header();

    if (argc < 2)
    {
        printf("Error: not enough arguments\n");
        printf("\tUsage: -h, --help\n");
        return fm;
    }

    // TODO: check that file names are valid

    size_t file_count = argc - 1;
    fm.buffers = new_elements(Buffer, file_count);
    if (fm.buffers)
    {
        fm.count = file_count;
        for (size_t i = 0; i < file_count; i++)
        {
            file_load_result = file_load(argv[i + 1], &fm.buffers[i]);
            if (file_load_result != FILE_LOAD_RESULT_SUCCESS)
            {
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
            memset(fm, 0, sizeof(FileManager));
        }
    }
}
