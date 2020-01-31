#pragma once

#include <list>
#include <mutex>

#include "crocore/crocore.hpp"

namespace crocore
{

//! forward declare a shared handle for a memory-pool
using BuddyPoolPtr = std::shared_ptr<class BuddyPool>;

/**
 * @brief   BuddyPool can be used to manage blocks of arbitrary memory using Buddy-allocations.
 *
 * @see     https://en.wikipedia.org/wiki/Buddy_memory_allocation
 *
 */
class BuddyPool
{
public:

    //! helper struct to group necessary information to create a BuddyPool.
    struct create_info_t
    {
        //! blocksize of toplevel blocks in bytes
        size_t blockSize;

        //! minimum blocksize in bytes
        size_t minBlockSize = 512;

        //! minimum number of preallocated blocks
        size_t minNumBlocks = 0;

        //! maximum number of blocks (default 0: unlimited)
        size_t maxNumBlocks = 0;

        //! function object to perform allocations with
        std::function<void*(size_t)> allocFn = ::malloc;

        //! function object to perform de-allocations with
        std::function<void(void*)> deallocFn = ::free;
    };

    //! helper struct to group relevant information about a BuddyPool's state.
    struct state_t
    {
        //! count of toplevel blocks currently allocated
        size_t numBlocks;

        //! blocksize of toplevel blocks in bytes
        size_t blockSize;

        //! maximum height for internal binary-tree, somewhat performance relevant.
        size_t maxLevel;

        //! maps allocation-sizes to counts
        std::map<size_t, size_t> allocations;
    };

    /**
     * @brief   Create a shared BuddyPool.
     *
     * @param   createInfo  a create_info_t struct.
     * @return  a newly created, shared BuddyPool.
     */
    static BuddyPoolPtr create(create_info_t createInfo);

    /**
     * @brief   Allocate memory from the pool.
     *          The amount of memory allocated will be rounded to the next power of two.
     *
     *          This call will return a nullptr if:
     *          - the requested numBytes is larger than the pool's blockSize.
     *          - there is no space in any existing toplevel block and the pool cannot allocate new blocks.
     *
     * @param   numBytes    the requested number of bytes to allocate.
     * @return  a pointer to the beginning of a memory-block or nullptr if the allocation failed.
     */
    void* allocate(size_t numBytes);

    /**
     * @brief   Free a block of memory, previously returned by this pool.
     *
     * @param   ptr a pointer to the beginning of a memory block, managed by this pool.
     */
    void free(void* ptr);

    /**
     * @brief   Query the current state of the pool.
     *
     * @return  a state_t object, grouping relevant state information.
     */
    state_t state();

private:

    explicit BuddyPool(create_info_t fmt);

    struct block_t createBlock();

    create_info_t _format;

    std::list<struct block_t> _topLevelBlocks;

    std::mutex _mutex;
};

}// namespace crocore


