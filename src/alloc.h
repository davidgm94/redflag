//
// Created by David on 04/10/2020.
//

#pragma once
#include "types.h"
#include "logger.h"
#include <malloc.h>
#include <string.h>


static inline void display_allocation_error(Allocation* allocation)
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
    T* ptr = nullptr;
    if (old)
    {
        ptr = reinterpret_cast<T*>(realloc(old, new_count * sizeof(T)));
    }
    else
    {
        ptr = reinterpret_cast<T*>(malloc(new_count * sizeof(T)));
    }
    if (!ptr)
        RED_PANIC("Reallocation not sucessful!");
    return ptr;
}
