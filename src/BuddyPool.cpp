#include <cassert>
#include <cstring>
#include <cmath>
#include "crocore/BuddyPool.hpp"


//! internal namespace for binary-tree utils
namespace tree
{
static inline size_t parent(size_t index){ return index > 0 ? (index + 1) / 2 - 1 : 0; }

static inline size_t left(size_t index){ return 2 * index + 1; }

static inline size_t right(size_t index){ return 2 * index + 2; }

static inline size_t buddy(size_t index)
{
    return index > 0 ? index - 1 + (index & 1U) * 2 : 0;
}

static inline size_t index_offset(size_t index, size_t level, size_t max_level)
{
    return ((index + 1) - (1U << level)) << (max_level - level);
}

}// namespace tree

namespace crocore
{

///////////////////////////////////////////////////////////////////////////////////////////////

enum class NodeState : uint8_t
{
    UNUSED = 0,
    USED = 1,
    SPLIT = 2,
    FULL = 3,
};

/**
 * @brief   block_t holds a block of memory along with a binary-tree for it's management.
 */
struct block_t
{
    uint32_t height = 0;
    std::unique_ptr<uint8_t, std::function<void(void *)>> data = nullptr;
    std::unique_ptr<NodeState[]> tree = nullptr;
};

//! create new binary tree
block_t buddy_create(size_t height);

/**
 * @brief   Perform an allocation in an existing block_t object.
 *
 * @param   b       a block_t object, containing a binary tree.
 * @param   size    the desired size for the allocation (unit is a sub-block of minBlockSize)
 * @return  the internal offset for the allocation (unit is a sub-block of minBlockSize),
 *          or -1, if the allocation could not be done.
 */
int buddy_alloc(block_t &b, size_t size);

void buddy_free(block_t &b, size_t offset);

void buddy_combine(block_t &b, size_t index);

void buddy_mark_parent(block_t &b, size_t index);

void buddy_collect_allocations(const block_t &b, size_t index, size_t level,
                               size_t minBlockSize, std::map<size_t, size_t> &allocations);

///////////////////////////////////////////////////////////////////////////////////////////////

block_t buddy_create(size_t height)
{
    size_t num_leaves = 1U << height;
    block_t ret = {};
    ret.height = height;
    ret.tree = std::unique_ptr<NodeState[]>(new NodeState[num_leaves * 2 - 1]);
    memset(ret.tree.get(), static_cast<uint8_t>(NodeState::UNUSED), num_leaves * 2 - 1);
    return ret;
}

void buddy_mark_parent(block_t &b, size_t index)
{
    for(;;)
    {
        size_t buddy = tree::buddy(index);

        if(buddy && (b.tree[buddy] == NodeState::USED || b.tree[buddy] == NodeState::FULL))
        {
            index = tree::parent(index);
            b.tree[index] = NodeState::FULL;
        }
        else{ return; }
    }
}

int buddy_alloc(block_t &b, size_t size)
{
    if(!size){ size = 1; }
    else{ size = next_pow_2(size); }

    // start with maximum number of leaves in tree
    size_t length = 1U << b.height;

    // requested size is too large
    if(size > length){ return -1; }

    size_t index = 0, level = 0;

    for(;;)
    {
        // found a matching block
        if(size == length)
        {
            if(b.tree[index] == NodeState::UNUSED)
            {
                b.tree[index] = NodeState::USED;
                buddy_mark_parent(b, index);

                return tree::index_offset(index, level, b.height);
            }
        }
        else
        {
            // size < length
            switch(b.tree[index])
            {
                case NodeState::USED:
                case NodeState::FULL:
                    break;

                case NodeState::UNUSED:
                    // split first
                    b.tree[index] = NodeState::SPLIT;
                    b.tree[tree::left(index)] = NodeState::UNUSED;
                    b.tree[tree::right(index)] = NodeState::UNUSED;

                case NodeState::SPLIT:
                    index = tree::left(index);
                    length /= 2;
                    level++;
                    continue;
            }
        }
        if(index & 1U)
        {
            ++index;
            continue;
        }

        // backtrack
        for(;;)
        {
            // arrived at root, nothing found
            if(!index){ return -1; }

            level--;
            length *= 2;
            index = tree::parent(index);

            if(index & 1U)
            {
                ++index;
                break;
            }
        }
    }
}

void buddy_combine(block_t &b, size_t index)
{
    for(;;)
    {
        size_t buddy = tree::buddy(index);

        if(!buddy || b.tree[buddy] != NodeState::UNUSED)
        {
            b.tree[index] = NodeState::UNUSED;

            index = tree::parent(index);

            while(tree::buddy(index) && b.tree[index] == NodeState::FULL)
            {
                b.tree[index] = NodeState::SPLIT;
                index = tree::parent(index);
            }
            return;
        }
        index = tree::parent(index);
    }
}

void buddy_free(block_t &b, size_t offset)
{
    assert(offset < (1U << b.height));

    size_t left = 0;
    size_t length = 1U << b.height;
    size_t index = 0;

    for(;;)
    {
        switch(b.tree[index])
        {
            case NodeState::USED:
                assert(offset == left);
                buddy_combine(b, index);
                return;

            case NodeState::UNUSED:
                assert(0);
                return;

            case NodeState::SPLIT:
            case NodeState::FULL:
                length /= 2;

                if(offset < left + length){ index = tree::left(index); }
                else
                {
                    left += length;
                    index = tree::right(index);
                }
                break;
        }
    }
}

void buddy_collect_allocations(const block_t &b, size_t index, size_t level,
                               size_t minBlockSize, std::map<size_t, size_t> &allocations)
{
    // current blocksize for this level
    size_t blockSize = minBlockSize << (b.height - level);

    switch(b.tree[index])
    {
        case NodeState::USED:
            allocations[blockSize]++;
            break;

        case NodeState::UNUSED:
            break;

        case NodeState::SPLIT:
        case NodeState::FULL:
            buddy_collect_allocations(b, tree::left(index), level + 1, minBlockSize, allocations);
            buddy_collect_allocations(b, tree::right(index), level + 1, minBlockSize, allocations);
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////

BuddyPoolPtr BuddyPool::create(create_info_t create_info)
{
    return BuddyPoolPtr(new BuddyPool(std::move(create_info)));
}

BuddyPool::BuddyPool(create_info_t fmt) :
        m_format(std::move(fmt))
{
    // enforce pow2 on all blocksizes, derive num_leaves
    m_format.block_size = next_pow_2(m_format.block_size);
    m_format.min_block_size = next_pow_2(m_format.min_block_size);

    // create toplevel blocks
    for(size_t i = 0; i < m_format.min_num_blocks; ++i)
    {
        m_toplevel_blocks.push_back(create_block());
    }
}

block_t BuddyPool::create_block()
{
    size_t max_level = std::log2(m_format.block_size / m_format.min_block_size);
    block_t new_block = buddy_create(max_level);

    // allocate the actual memory to be managed
    if(m_format.alloc_fn && m_format.dealloc_fn)
    {
        new_block.data = std::unique_ptr<uint8_t, std::function<void(void *)>>(
                (uint8_t *) m_format.alloc_fn(m_format.block_size),
                m_format.dealloc_fn);
    }
    return new_block;
}

void *BuddyPool::allocate(size_t num_bytes)
{
    // requested numBytes is zero or too large
    if(!num_bytes || num_bytes > m_format.block_size){ return nullptr; }

    std::unique_lock<std::mutex> lock(m_mutex);

    // derive number of minimum blocks required
    size_t size = std::ceil(num_bytes / (float) m_format.min_block_size);

    // iterate toplevel blocks
    for(auto &b : m_toplevel_blocks)
    {
        // within block, try to allocate, recursively split, find proper index
        int allocation_index = buddy_alloc(b, size);

        // allocation succeeded
        if(allocation_index >= 0)
        {
            return b.data.get() + allocation_index * m_format.min_block_size;
        }
    }

    // add new toplevel block, if maxNumBlocks permits it
    if(!m_format.max_num_blocks || m_toplevel_blocks.size() < m_format.max_num_blocks)
    {
        auto new_block = create_block();

        int allocation_index = buddy_alloc(new_block, size);

        // this here should always work
        if(allocation_index >= 0)
        {
            auto ptr = new_block.data.get() + allocation_index * m_format.min_block_size;
            m_toplevel_blocks.push_back(std::move(new_block));
            return ptr;
        }
    }
    // no free block with sufficient size could be found or created
    return nullptr;
}

void BuddyPool::free(void *ptr)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    // find proper toplevel block
    auto blockIter = m_toplevel_blocks.begin();

    for(; blockIter != m_toplevel_blocks.end(); ++blockIter)
    {
        auto &b = *blockIter;

        auto block_end = b.data.get() + m_format.block_size;

        // address is inside this block
        if(ptr >= b.data.get() && ptr < block_end)
        {
            auto ptr_offset = static_cast<uint8_t *>(ptr) - b.data.get();

            // invalid address
            if(ptr_offset % m_format.min_block_size){ return; }

            // calculate index-offset from address
            size_t index_offset = ptr_offset / m_format.min_block_size;

            // recursive free / combine blocks
            buddy_free(b, index_offset);

            // de-allocate unused blocks above minNumBlocks
            if(m_format.dealloc_unused_blocks && b.tree[0] == NodeState::UNUSED &&
               m_toplevel_blocks.size() > m_format.min_num_blocks)
            {
                m_toplevel_blocks.erase(blockIter);
            }
            break;
        }
    }
}

void BuddyPool::shrink()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    // iterate toplevel blocks
    auto blockIter = m_toplevel_blocks.begin();

    for(; blockIter != m_toplevel_blocks.end(); ++blockIter)
    {
        auto &b = *blockIter;

        // de-allocate unused blocks above minNumBlocks
        if(b.tree[0] == NodeState::UNUSED && m_toplevel_blocks.size() > m_format.min_num_blocks)
        {
            blockIter = m_toplevel_blocks.erase(blockIter);
        }
    }
}

BuddyPool::state_t BuddyPool::state()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    BuddyPool::state_t ret = {};
    ret.num_blocks = m_toplevel_blocks.size();
    ret.block_size = m_format.block_size;
    ret.max_level = std::log2(m_format.block_size / m_format.min_block_size);

    for(const auto &b : m_toplevel_blocks)
    {
        buddy_collect_allocations(b, 0, 0, m_format.min_block_size, ret.allocations);
    }
    return ret;
}

}// namespace crocore