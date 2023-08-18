//
// Created by crocdialer on 2/28/20.
//

#pragma once

#include <crocore/define_class_ptr.hpp>


namespace crocore
{

//! forward declare smart-pointers for an Allocator
DEFINE_CLASS_PTR(Allocator)

class Allocator
{
public:
    /**
     * @brief   state_t is an aggregate,
     *          grouping information about the current state of an Allocator.
     */
    struct state_t
    {
        //! total number of internal allocations held.
        size_t num_allocations = 0;

        //! total number of internally allocated bytes.
        size_t num_bytes_allocated = 0;

        //! total number of bytes in active (client-) allocations.
        size_t num_bytes_used = 0;
    };

    /**
     * @brief   Allocate a contiguous block of memory.
     *
     * @param   numBytes    the requested number of bytes to allocate.
     * @return  a pointer to the beginning of a memory-block or nullptr if the allocation failed.
     */
    virtual void *allocate(size_t numBytes) = 0;

    /**
     * @brief   Free a block of memory, previously returned by this allocator.
     *
     * @param   ptr a pointer to the beginning of a memory block, managed by this allocator.
     */
    virtual void free(void *ptr) = 0;

    /**
     * @brief   Shrinks the internally allocated memory to a minimum, without affecting existing allocations.
     */
    virtual void shrink() = 0;

    /**
     * @brief   Return a summary of the allocator's internal state.
     */
    [[nodiscard]] virtual Allocator::state_t state() const = 0;
};

template<class T>
struct stl_allocator
{
    using value_type = T;

    explicit stl_allocator(AllocatorPtr allocator_ = nullptr) : allocator(std::move(allocator_)) {}

    template<class U>
    constexpr explicit stl_allocator(const stl_allocator<U> &other) noexcept : allocator(other.allocator)
    {}

    [[nodiscard]] T *allocate(std::size_t n)
    {
        if(n > std::numeric_limits<std::size_t>::max() / sizeof(T)) throw std::bad_array_new_length();
        T *ptr = allocator ? static_cast<T *>(allocator->allocate(n * sizeof(T))) : nullptr;
        if(ptr) { return ptr; }
        throw std::bad_alloc();
    }

    void deallocate(T *p, std::size_t) noexcept { allocator->free(p); }

    AllocatorPtr allocator;
};

}// namespace crocore

/**
 * @brief       Overload for performing a "placement-new".
 *
 *              example-usage:
 *
 *              ```````````````````````````````````
 *              Foo* foo = new (myAllocator) Foo();
 *
 *              foo->~Foo();
 *              myAllocator->free(foo);
 *              ```````````````````````````````````
 */
inline void *operator new(size_t num_bytes, const crocore::AllocatorPtr &allocator)
{
    return allocator->allocate(num_bytes);
}

/**
 * @brief   Used only implicitly in combination with above placement-new.
 *          Do not invoke delete on memory obtained from an uf::Allocator.
 *          Purpose is solely exception-safe usage of above operator-new.
 */
inline void operator delete(void *ptr, const crocore::AllocatorPtr &allocator) { allocator->free(ptr); }
