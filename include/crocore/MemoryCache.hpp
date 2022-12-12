//=============================================================================================
//  Copyright (c) 2020 uniqFEED Ltd. All rights reserved.
//=============================================================================================

#pragma once

#include <map>
#include <mutex>
#include <functional>
#include <crocore/Allocator.hpp>


namespace crocore
{

//! forward declare a shared handle for a memory-pool
using MemoryCachePtr = std::shared_ptr<class MemoryCache>;

/**
 * @brief   MemoryCache is an implementation of uf::Allocator, using a caching-strategy.
 */
class MemoryCache final : public Allocator
{
public:

    //! helper struct to group necessary information to create an MemoryCache.
    struct create_info_t
    {
        //! minimum size in bytes for an allocation (defaults to 4kB)
        size_t min_size = 1U << 12U;

        //! maximum size-tolerance for recycling free chunks
        float size_tolerance = 2.f;

        //! function object to perform allocations with
        std::function<void *(size_t)> alloc_fn = ::malloc;

        //! function object to perform de-allocations with
        std::function<void(void *)> dealloc_fn = ::free;
    };

    static MemoryCachePtr create(create_info_t fmt);

    ~MemoryCache();

    /**
     * @brief   Allocate memory from the cache.
     *
     *          Will search the map of free chunks for a memory-block that is large enough.
     *          If nothing was found a new block of memory will be allocated.
     *
     * @param   numBytes    the requested number of bytes to allocate.
     * @return  a pointer to the beginning of a memory-block or nullptr if the allocation failed.
     */
    void *allocate(size_t numBytes) override;

    /**
     * @brief   Free a block of memory, previously returned by this cache.
     *          The block will be kept in the map of free chunks for later re-use.
     *
     * @param   ptr a pointer to the beginning of a memory block, managed by this MemoryCache.
     */
    void free(void *ptr) override;

    /**
     * @brief   Shrinks the internally allocated memory to a minimum, without affecting existing allocations.
     *          In case of a MemoryCache, this will free all unused memory-chunks.
     *
     */
    void shrink() override;

    /**
     * @brief   Return a summary of the allocator's internal state.
     */
    Allocator::state_t state() const;

private:

    //! map type for used memory
    using used_ptr_map_t = std::map<void *, size_t>;

    //! map type for free memory
    using free_ptr_map_t = std::multimap<size_t, void *>;

    explicit MemoryCache(create_info_t fmt);

    create_info_t _format;
    free_ptr_map_t _freeChunks;
    used_ptr_map_t _usedChunks;
    mutable std::recursive_mutex _mutex;
};
}


