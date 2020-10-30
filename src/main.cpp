//
// Created by David on 04/10/2020.
//

#if 1
#include "types.h"
#include "lexer.h"
#include "file.h"
#include "src_file.h"
#include "os.h"

struct FileManager
{
    Buffer* buffers;
    char** filenames;
    size_t count;
};

static FileManager handle_main_arguments(s32 argc, char* argv[]);
static void FileManager_cleanup(FileManager* fm);

#if RED_TIMESTAMPS
#endif

s32 main(s32 argc, char* argv[])
{
    os_init(mem_init);
    s64 start = os_performance_counter();

    FileManager fm = handle_main_arguments(argc, argv);
    if (fm.count == 0)
        return 0;


    for (size_t i = 0; i < fm.count; i++)
    {
        Buffer* src_buffer = &fm.buffers[i];
        add_source_file(src_buffer, argv[i + 1]);
    }

    FileManager_cleanup(&fm);
    s64 end = os_performance_counter();
    f64 total_ms = os_compute_ms(start, end);
    os_print_recorded_times(total_ms);

    return 0;
}

//static inline void print_header(void)
//{
//    print("Red language compiler\n");
//}

static FileManager handle_main_arguments(s32 argc, char* argv[])
{
    Buffer* cwd = buf_alloc();
    os_get_cwd(cwd);
#if RED_CWD_VERBOSE
    print("CWD: %s\n", buf_ptr(cwd));
#endif
    FileManager fm = {0};
    FileLoadResult file_load_result;
    //print_header();

    if (argc < 2)
    {
        print("Error: not enough arguments\n");
        print("\tUsage: -h, --help\n");
        return fm;
    }

    // TODO: check that file names are valid

    size_t file_count = argc - 1;
    fm.buffers = NEW<Buffer>(file_count);
    fm.filenames = NEW<char*>(file_count);
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
#else

#include "types.h"
#include <Windows.h>
#include <assert.h>
#include <stdio.h>

struct Buffer
{
    u8* memory;
    s64 occupied;
    s64 capacity;

    template<typename T>
    void append(T value)
    {
        assert(this->occupied + sizeof(T) <= this->capacity);
        T* write_ptr = (T*)&this->memory[this->occupied];
        *write_ptr = value;
        this->occupied += sizeof(T);
    }

    void append_sub_rsp_imm_8(s8 value)
    {
        append((u8)0x48);
        append((u8)0x83);
        append((u8)0xec);
        append(value);
    }

    void append_mov_to_stack_offset_imm_32(s32 value)
    {
        append((u8)0xc7);
        append((u8)0x04);
        append((u8)0x24);
        append(value);
        append((u8)value);
    }
};

Buffer make_buffer(s64 capacity, s32 mem_flags)
{
    u8* memory = (u8*)VirtualAlloc(nullptr, capacity, MEM_COMMIT | MEM_RESERVE, mem_flags);
    assert(memory);
    return { memory, 0, capacity };
}


static void ptest(const char* text, bool expr)
{
    printf("%s %s\n", text, expr ? "OK" : "FAIL");
}
#define constant static constexpr const
using get_constant_s32 = s32(s32 value);
using identity_s64 = s64(void);

constant u8 x64_ret = 0xc3;

get_constant_s32* make_constant_s32(s32 value)
{
    Buffer buffer = make_buffer(1024, PAGE_EXECUTE_READWRITE);
    buffer.append((u8)0x48);
    buffer.append((u8)0xc7);
    buffer.append((u8)0xc0);
    buffer.append(value);
    buffer.append(x64_ret);
    return (get_constant_s32*)buffer.memory;
}

identity_s64* make_identity_s64()
{
    Buffer buffer = make_buffer(1024, PAGE_EXECUTE_READWRITE);
    buffer.append((u8)0x48);
    buffer.append((u8)0x89);
    buffer.append((u8)0xc8);
    buffer.append(x64_ret);
    return (identity_s64*)buffer.memory;
}

using increment_s64 = s64();

increment_s64* make_increment_s64()
{
    Buffer buffer = make_buffer(1024, PAGE_READWRITE);
    buffer.append_sub_rsp_imm_8(24);
    //buffer.append((u8)0x48);
    //buffer.append((u8)0x89);
    //buffer.append((u8)0xc8);
    buffer.append(x64_ret);
    return (increment_s64*)buffer.memory;
}

int main(void)
{
    s32 value = 5;
    get_constant_s32* make_const = make_constant_s32(5);
    s32 value_test = make_const(5);
    ptest("make_const", value == value_test);
}

#endif
