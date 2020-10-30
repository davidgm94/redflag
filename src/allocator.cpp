#include "compiler_types.h"
#include "os.h"
#include <stdlib.h>

static constexpr usize cache_line_size = 64 * A_BYTE;
static constexpr usize max_allocated_block_count = 4;
static constexpr usize block_alignment = cache_line_size;
static constexpr usize block_size = 4 * GIGABYTE;
static constexpr usize default_alignment = 16;

struct MemoryBlock
{
    void* address;
    void* aligned_address;
    void* available_address;
};

struct Allocation
{
    u32 alignment;
    u32 size;
};

struct PageAllocator
{
    void* available_address;
    usize allocation_count;
    usize allocated_block_count;
    void* blob;
    usize page_size;
};

static PageAllocator m_page_allocator;

enum AllocationResult
{
    Error,
    Success,
};

void mem_init(void)
{
    m_page_allocator.page_size = os_get_page_size();
}

// @NOT_USED
//static inline usize round_up_to_next_page(usize size, usize page_size)
//{
//    usize remainder = size % page_size;
//    usize to_add = page_size - remainder;
//    return size + to_add;
//}

static inline void* align_address(void* address, usize align)
{
    const size_t mask = align - 1;
    uptr p = (uptr)address;
    redassert((align & mask) == 0);

    return (void*)((p + mask) & ~mask);
}

static inline void fill_with_allocation_garbage(void* start, void* end)
{
    u8* it = (u8*)start;
    while (it != end)
    {
        *it++ = 0xff;
    }
}

static inline Allocation* find_allocation_metadata(void* visible_address)
{
    u8* it = (u8*)((uptr)visible_address - sizeof(Allocation));
    u32* alignment_ptr = (u32*)it;
    redassert(*alignment_ptr == 16);
    redassert(it < (u8*)visible_address);
    return (Allocation*)it;
}

//static inline void buffer_zero_check(void* address, size_t size)
//{
//    if (address)
//    {
//        u8* it = (u8*)address;
//        for (usize i = 0; i < size; i++)
//        {
//            if (it[i] != 0)
//            {
//                redassert(it[i] == 0);
//            }
//        }
//    }
//}

static inline void allocate_new_block()
{
    void* target_address = nullptr;
    if (m_page_allocator.allocation_count > 0)
    {
        void* original_address = m_page_allocator.blob;
        target_address = (void*)((uptr)original_address * (m_page_allocator.allocation_count + 1));
    }
    void* address = os_ask_virtual_memory_block_with_address(target_address, block_size);
    redassert(address != nullptr);
    if (target_address)
    {
        redassert(address == target_address);
    }

#if RED_BUFFER_MEM_CHECK
    buffer_zero_check(address, block_size);
#endif

    if (address)
    {
        void* aligned_address = align_address(address, block_alignment);
        redassert(aligned_address == address);
        usize lost_memory = (uptr)aligned_address - (uptr)address;
        redassert(lost_memory == 0);
        usize aligned_size = block_size - lost_memory;
        redassert(aligned_size == block_size);
        if (target_address == nullptr)
        {
            m_page_allocator.blob = address;
            m_page_allocator.available_address = address;
        }
        else
        {
            m_page_allocator.available_address = target_address;
        }
        m_page_allocator.allocated_block_count++;
    }
    else
    {
        logger(LOG_TYPE_ERROR, "Block allocation failed!\n");
        abort();
    }
}

static inline uptr top_address(void)
{
    return (uptr)m_page_allocator.blob + (m_page_allocator.allocated_block_count * block_size);
}

void* allocate_chunk(usize size)
{
#if RED_BUFFER_MEM_CHECK
    buffer_zero_check(m_page_allocator.available_address, (uptr)m_page_allocator.blob +  block_size - (uptr)m_page_allocator.available_address);
#endif
    redassert(m_page_allocator.allocated_block_count <= max_allocated_block_count);
    // TODO:
    usize max_required_size = size + default_alignment;
    redassert(sizeof(Allocation) < default_alignment);

    bool first_allocation = m_page_allocator.allocated_block_count == 0;
    bool can_allocate_in_current_block = !first_allocation;

    if (!first_allocation)
    {
        uptr available_address = (uptr)m_page_allocator.available_address;
        usize free_space = top_address() - available_address;
        bool no_need_to_allocate_another_block = max_required_size < free_space;
        can_allocate_in_current_block = can_allocate_in_current_block && no_need_to_allocate_another_block;
    }

    if (!can_allocate_in_current_block)
    {
        allocate_new_block();
    }

    // This is the address with the allocation metadata
    void* address = m_page_allocator.available_address;
    void* aligned_address = align_address((void*)((uptr)address + sizeof(Allocation)), default_alignment);

    fill_with_allocation_garbage(address, aligned_address);

    usize pointer_displacement = (uptr)aligned_address - (uptr)address + size;
    uptr new_available_address_number = (uptr)address + pointer_displacement;
    void* new_available_address = (void*)new_available_address_number;
    redassert(new_available_address_number < top_address());
    Allocation* allocation = (Allocation*)((uptr)aligned_address - sizeof(Allocation));
    static_assert(sizeof(Allocation) == (sizeof(u32) * 2), "Wrong allocation packing");
    allocation->alignment = default_alignment;
    allocation->size = pointer_displacement;
    redassert(allocation->size != 0);
    m_page_allocator.available_address = new_available_address;
    m_page_allocator.allocation_count++;

#if RED_BUFFER_MEM_CHECK
    buffer_zero_check(aligned_address, size);
#endif
#if RED_ALLOCATION_VERBOSE
    print("[ALLOCATOR] (#%zu) Allocating %zu bytes at address 0x%p. Total allocated: %zu\n", m_page_allocator.allocation_count, allocation->size, address, (usize)((uptr)m_page_allocator.available_address - (uptr)m_page_allocator.blob));
#endif
    redassert(new_available_address != address);

#if RED_BUFFER_MEM_CHECK
    buffer_zero_check(m_page_allocator.available_address, (uptr)m_page_allocator.blob +  block_size - (uptr)m_page_allocator.available_address);
#endif
    return (void*)aligned_address;
}

// TODO: don't overload new for now
// void* operator new(size_t size)
// {
//     void* address;
//     if (m_page_allocator.page_size)
//     {
//         address = NEW<u8>(size);
//     }
//     else
//     {
//         address = os_ask_heap_memory(size);
//     }
// #if ALLOCATION_VERBOSE
//     logger(LOG_TYPE_INFO, "Allocation made through operator new: %zu bytes at address 0x%p\n", size, address);
// #endif
//     return address;
// }

void* reallocate_chunk(void* allocated_address, usize size)
{
    redassert(size > 0);
    redassert(size < UINT32_MAX);
    if (!allocated_address)
    {
        return allocate_chunk(size);
    }
    
    Allocation* allocation_metadata = find_allocation_metadata(allocated_address);
    usize difference = (uptr)allocated_address - (uptr)allocation_metadata;
    usize real_size = allocation_metadata->size - difference;
    redassert(real_size < size);
    void* new_address = allocate_chunk(size);
    memcpy(new_address, allocated_address, real_size);

    return new_address;
}
