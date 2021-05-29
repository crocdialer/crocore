#pragma once

#include <list>
#include <map>
#include <mutex>
#include <shared_mutex>

#include "crocore/crocore.hpp"
#include "crocore/Allocator.hpp"

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
class BuddyPool final : public crocore::Allocator
{
public:

    //! helper struct to group necessary information to create a BuddyPool.
    struct create_info_t
    {
        //! blocksize of toplevel blocks in bytes
        size_t block_size;

        //! minimum blocksize in bytes
        size_t min_block_size = 512;

        //! minimum number of preallocated blocks
        size_t min_num_blocks = 0;

        //! maximum number of blocks (default 0: unlimited)
        size_t max_num_blocks = 0;

        //! enable automatic deallocation of unused blocks
        bool dealloc_unused_blocks = true;

        //! function object to perform allocations with
        std::function<void *(size_t)> alloc_fn = ::malloc;

        //! function object to perform de-allocations with
        std::function<void(void *)> dealloc_fn = ::free;
    };

    //! helper struct to group relevant information about a BuddyPool's state.
    struct pool_state_t
    {
        //! count of toplevel blocks currently allocated
        size_t num_blocks;

        //! blocksize of toplevel blocks in bytes
        size_t block_size;

        //! maximum height for internal binary-tree, somewhat performance relevant.
        size_t max_level;

        //! maps allocation-sizes to counts
        std::map<size_t, size_t> allocations;
    };

    /**
     * @brief   Create a shared BuddyPool.
     *
     * @param   create_info  a create_info_t struct.
     * @return  a newly created, shared BuddyPool.
     */
    static BuddyPoolPtr create(create_info_t create_info);

    /**
     * @brief   Allocate memory from the pool.
     *          The amount of memory allocated will be rounded to the next power of two.
     *
     *          This call will return a nullptr if:
     *          - the requested numBytes is larger than the pool's blockSize.
     *          - there is no space in any existing toplevel block and the pool cannot allocate new blocks.
     *
     * @param   num_bytes    the requested number of bytes to allocate.
     * @return  a pointer to the beginning of a memory-block or nullptr if the allocation failed.
     */
    void *allocate(size_t num_bytes) override;

    /**
     * @brief   Free a block of memory, previously returned by this pool.
     *
     * @param   ptr a pointer to the beginning of a memory block, managed by this pool.
     */
    void free(void *ptr) override;

    /**
     * @brief   Shrinks the internally allocated memory to a minimum, without affecting existing allocations.
     *          In case of a BuddyPool, unused toplevel-blocks above create_info_t::min_num_blocks will be de-allocated.
     *
     */
    void shrink() override;


    /**
     * @brief   Return a summary of the allocator's internal state.
     */
    [[nodiscard]] Allocator::state_t state() const override;

    /**
     * @brief   Query the current state of the pool.
     *
     * @return  a state_t object, grouping relevant state information.
     */
    [[nodiscard]] pool_state_t pool_state() const;

private:

    explicit BuddyPool(create_info_t fmt);

    struct block_t create_block() const;

    create_info_t m_format;

    std::list<struct block_t> m_toplevel_blocks;

    mutable std::shared_mutex m_mutex;
};

}// namespace crocore


