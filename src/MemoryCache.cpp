//=============================================================================================
//  Copyright (c) 2020 uniqFEED Ltd. All rights reserved.
//=============================================================================================

#include <crocore/MemoryCache.hpp>


namespace crocore
{

MemoryCachePtr MemoryCache::create(create_info_t fmt)
{
    return crocore::MemoryCachePtr(new MemoryCache(std::move(fmt)));
}

MemoryCache::MemoryCache(create_info_t fmt) :
        _format(std::move(fmt))
{

}

MemoryCache::~MemoryCache()
{
    // use std::try_to_lock to avoid a possible exception
    std::unique_lock lock(_mutex, std::try_to_lock);

    if(_format.dealloc_fn)
    {
        for(auto &it: _freeChunks){ _format.dealloc_fn(it.second); }
        for(auto &it: _usedChunks){ _format.dealloc_fn(it.first); }
    }
}

void *MemoryCache::allocate(size_t num_bytes)
{
    if(!num_bytes){ return nullptr; }

    num_bytes = std::max(num_bytes, _format.min_size);

    std::unique_lock lock(_mutex);

    // upper bound for the accepted size of a recycled chunk
    size_t maxNumBytes = num_bytes * std::max(_format.size_tolerance, 1.f);

    // search for a lower bound (equal or greater)
    auto it = _freeChunks.lower_bound(num_bytes);

    if(it != _freeChunks.end() && it->first <= maxNumBytes)
    {
        void *ptr = it->second;
        _usedChunks[ptr] = it->first;
        _freeChunks.erase(it);
        return ptr;
    }

    // only allocate if we can also de-allocate
    if(_format.alloc_fn && _format.dealloc_fn)
    {
        void *ptr = _format.alloc_fn(num_bytes);

        if(!ptr)
        {
            shrink();
            ptr = _format.alloc_fn(num_bytes);
        }

        if(ptr){ _usedChunks[ptr] = num_bytes; }
        return ptr;
    }

    // no allocators available
    return nullptr;
}

void MemoryCache::free(void *ptr)
{
    if(!ptr){ return; }

    std::unique_lock lock(_mutex);

    /// Remove pointer from lock map, if it exists
    auto it = _usedChunks.find(ptr);

    if(it != _usedChunks.end())
    {
        _freeChunks.insert(std::make_pair(it->second, it->first));
        _usedChunks.erase(it);
    }
}

void MemoryCache::shrink()
{
    std::unique_lock lock(_mutex);

    // free memory chunks and clear map
    for(auto &it: _freeChunks)
    {
        if(_format.dealloc_fn){ _format.dealloc_fn(it.second); }
    }
    _freeChunks.clear();
}

Allocator::state_t MemoryCache::state() const
{
    std::unique_lock lock(_mutex);

    Allocator::state_t ret = {};
    ret.num_allocations = _freeChunks.size() + _usedChunks.size();

    // collect currently used chunks
    for(const auto &[ptr, numBytes]: _usedChunks){ ret.num_bytes_used += numBytes; }

    ret.num_bytes_allocated = ret.num_bytes_used;

    // collect idle/free chunks
    for(const auto &[numBytes, ptr]: _freeChunks){ ret.num_bytes_allocated += numBytes; }

    return ret;
}
}
