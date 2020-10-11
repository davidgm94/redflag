//
// Created by David on 04/10/2020.
//

#ifndef REDFLAG_ALLOC_H
#define REDFLAG_ALLOC_H
#include "types.h"
#include "memory.h"
#include <malloc.h>
#include <assert.h>
#include "type_info.h"
#include "logger.h"
#include "error.h"

static constexpr size_t max_filename_length = 512;
struct Allocation
{
    char file[max_filename_length];
    char function[max_filename_length];
    char type[max_filename_length];
    TypeInfo type_info;
    void* address;
    size_t line;
};
static constexpr size_t MAX_ALLOCS = 1024 * 100;
struct Allocator
{
    Allocation allocations[MAX_ALLOCS];
    size_t allocation_count;
};
static Allocator allocator = {};

static void display_allocation_error(Allocation* allocation)
{
    logger(LOG_TYPE_ERROR, "Error in %s allocation at (%s:%zu:%s). Size: %zu. Alignment: %zu.\n",
           allocation->type, allocation->file, allocation->line, allocation->function, allocation->type_info.size, allocation->type_info.alignment);
}

template <typename T>
static inline T* register_allocation(const char* type, const char* filename, size_t line, const char* function_name, size_t size)
{
    Allocation allocation = {};
    void* address = malloc(size);
    strcpy(allocation.file, filename);
    strcpy(allocation.function, function_name);
    strcpy(allocation.type, type);
    allocation.type_info = TypeInfo::make<T>();
    allocation.address = address;
    allocation.line = line;

    if (address != nullptr)
    {
        memcpy(&allocator.allocations[allocator.allocation_count++], &allocation, sizeof(allocation));
        return reinterpret_cast<T*>(address);
    }
    else
    {
        display_allocation_error(&allocation);
        return nullptr;
    }
}

#define new_elements(type, count) alloc_size<type>(#type, __FILE__, __LINE__, __FUNCTION__, count * sizeof(type))
#define new_block(type, size) alloc_size<type>(#type, __FILE__, __LINE__, __FUNCTION__, size)
template <typename T>
static inline T* alloc_size(const char* type, const char* filename, size_t line, const char* function_name, size_t size)
{
    return register_allocation<T>(type, filename, line, function_name, size);
}

template<typename T>
static inline T *reallocate_nonzero(T *old, size_t new_count) {
    T* ptr = reinterpret_cast<T*>(realloc(old, new_count * sizeof(T)));
    if (!ptr)
        RED_PANIC("realloc error");
    return ptr;
}


#endif //REDFLAG_ALLOC_H
