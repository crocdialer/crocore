//
// Created by crocdialer on 2/28/20.
//

#pragma once

#include <cstring>
#include <memory>


namespace crocore
{

//! forward declare a shared handle for an Allocator
using AllocatorPtr = std::shared_ptr<class Allocator>;


class Allocator
{
public:

    /**
     * @brief   Allocate a contiguous block of memory.
     *
     * @param   numBytes    the requested number of bytes to allocate.
     * @return  a pointer to the beginning of a memory-block or nullptr if the allocation failed.
     */
    virtual void* allocate(size_t numBytes) = 0;

    /**
     * @brief   Free a block of memory, previously returned by this allocator.
     *
     * @param   ptr a pointer to the beginning of a memory block, managed by this allocator.
     */
    virtual void free(void* ptr) = 0;

    /**
     * @brief   Shrinks the internally allocated memory to a minimum, without affecting existing allocations.
     *
     */
    virtual void shrink() = 0;
};
}
