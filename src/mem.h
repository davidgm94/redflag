//
// Created by david on 10/17/20.
//

#pragma once
#include <malloc.h>
#include "types.h"
#include "panic.h"

namespace mem {

// initialize mem module before any use
    void init();

// deinitialize mem module to free memory and print report
    void deinit();

// isolate system/libc allocators
    namespace os {

        inline void *malloc(size_t size) {
#ifndef NDEBUG
            // make behavior when size == 0 portable
            if (size == 0)
                return nullptr;
#endif
            auto ptr = ::malloc(size);
            if (ptr == nullptr)
                RED_PANIC("allocation failed");
            return ptr;
        }

        inline void free(void *ptr) {
            ::free(ptr);
        }

        inline void *calloc(size_t count, size_t size) {
#ifndef NDEBUG
            // make behavior when size == 0 portable
            if (count == 0 || size == 0)
                return nullptr;
#endif
            auto ptr = ::calloc(count, size);
            if (ptr == nullptr)
                RED_PANIC("allocation failed");
            return ptr;
        }

        inline void *realloc(void *old_ptr, size_t size) {
#ifndef NDEBUG
            // make behavior when size == 0 portable
            if (old_ptr == nullptr && size == 0)
                return nullptr;
#endif
            auto ptr = ::realloc(old_ptr, size);
            if (ptr == nullptr)
                RED_PANIC("allocation failed");
            return ptr;
        }

    } // namespace os

    struct Allocator {
        virtual void destruct(Allocator *allocator) = 0;

        template <typename T>
                T *allocate(size_t count) {
            return reinterpret_cast<T *>(this->internal_allocate(TypeInfo::make<T>(), count));
        }

        template <typename T>
                T *allocate_nonzero(size_t count) {
            return reinterpret_cast<T *>(this->internal_allocate_nonzero(TypeInfo::make<T>(), count));
        }

        template <typename T>
        T *reallocate(T *old_ptr, size_t old_count, size_t new_count) {
            return reinterpret_cast<T *>(this->internal_reallocate(TypeInfo::make<T>(), old_ptr, old_count, new_count));
        }

        template <typename T>
        T *reallocate_nonzero(T *old_ptr, size_t old_count, size_t new_count) {
            return reinterpret_cast<T *>(this->internal_reallocate_nonzero(TypeInfo::make<T>(), old_ptr, old_count, new_count));
        }

        template<typename T>
        void deallocate(T *ptr, size_t count) {
            this->internal_deallocate(TypeInfo::make<T>(), ptr, count);
        }

        template<typename T>
        T *create() {
            return reinterpret_cast<T *>(this->internal_allocate(TypeInfo::make<T>(), 1));
        }

        template<typename T>
        void destroy(T *ptr) {
            this->internal_deallocate(TypeInfo::make<T>(), ptr, 1);
        }

    protected:
        virtual void *internal_allocate(const TypeInfo &info, size_t count) = 0;
        virtual void *internal_allocate_nonzero(const TypeInfo &info, size_t count) = 0;
        virtual void *internal_reallocate(const TypeInfo &info, void *old_ptr, size_t old_count, size_t new_count) = 0;
        virtual void *internal_reallocate_nonzero(const TypeInfo &info, void *old_ptr, size_t old_count, size_t new_count) = 0;
        virtual void internal_deallocate(const TypeInfo &info, void *ptr, size_t count) = 0;
    };

} // namespace mem
