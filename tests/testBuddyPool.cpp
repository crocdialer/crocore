#include <gtest/gtest.h>

#include "crocore/BuddyPool.hpp"
#include <unordered_map>

static inline bool is_pow_2(size_t v)
{
    return !(v & (v - 1));
}

static inline size_t next_pow_2(size_t v)
{
    if(is_pow_2(v)){ return v; }
    v--;
    v |= v >> 1U;
    v |= v >> 2U;
    v |= v >> 4U;
    v |= v >> 8U;
    v |= v >> 16U;
    v |= v >> 32U;
    v++;
    return v;
}


TEST(BuddyPool, Constructors)
{
    constexpr size_t numBytes_128Mb = 1U << 27U;

    // no pre-allocation
    {
        crocore::BuddyPool::create_info_t createInfo;

        // 128MB - something
        createInfo.block_size = numBytes_128Mb - 12345;
        createInfo.min_block_size = 512;

        // check for existence of default allocator/de-allocator (malloc/free)
        ASSERT_TRUE(createInfo.alloc_fn);
        ASSERT_TRUE(createInfo.dealloc_fn);

        auto pool = crocore::BuddyPool::create(createInfo);

        auto pool_state = pool->pool_state();

        // nothing was pre-allocated
        ASSERT_EQ(pool_state.num_blocks, 0);

        // we have passed a non pow2 -> check for correct rounding to next pow2
        ASSERT_EQ(pool_state.block_size, numBytes_128Mb);

        // maximum height of binary tree
        ASSERT_EQ(pool_state.max_level, 18);
    }

    // with pre-allocation
    {
        constexpr size_t minNumBlocks = 2;

        crocore::BuddyPool::create_info_t createInfo = {};

        // 128MB - something
        createInfo.block_size = numBytes_128Mb - 54321;
        createInfo.min_block_size = 2048;

        // request a minimum of blocks to be around -> pre-allocation
        createInfo.min_num_blocks = minNumBlocks;

        auto pool = crocore::BuddyPool::create(createInfo);

        auto pool_state = pool->pool_state();

        // check for pre-allocated blocks
        ASSERT_EQ(pool_state.num_blocks, minNumBlocks);

        // we have passed a non pow2 -> check for correct rounding to next pow2
        ASSERT_EQ(pool_state.block_size, numBytes_128Mb);

        // maximum height of binary tree
        ASSERT_EQ(pool_state.max_level, 16);
    }
}


TEST(BuddyPool, Allocations)
{
    constexpr size_t numBytes_16Mb = 1U << 24U;
    constexpr size_t numBytes_1Mb = 1U << 20U;

    crocore::BuddyPool::create_info_t fmt;

    // 16MB
    fmt.block_size = numBytes_16Mb;
    fmt.min_block_size = 512;

    // preallocate one block
    fmt.min_num_blocks = 0;

    // no automatic deallocation
    fmt.dealloc_unused_blocks = false;

    auto pool = crocore::BuddyPool::create(fmt);

    // allocate entire toplevel block
    void *ptr1 = pool->allocate(numBytes_16Mb);
    ASSERT_NE(ptr1, nullptr);

    // free pointer
    pool->free(ptr1);

    // 1MB allocation
    void *ptr2 = pool->allocate(numBytes_1Mb);

    // allocation succeeded
    ASSERT_NE(ptr2, nullptr);

    // should have been assigned the same offset like before
    ASSERT_EQ(ptr1, ptr2);

    auto pool_state = pool->pool_state();

    // expect exactly one 1MB-block here
    ASSERT_EQ(pool_state.allocations.size(), 1);
    ASSERT_EQ(pool_state.allocations[numBytes_1Mb], 1);

    // free pointer
    pool->free(ptr2);

    // expect an empty pool
    pool_state = pool->pool_state();
    ASSERT_EQ(pool_state.allocations.size(), 0);

    // test zero byte allocation fails
    ASSERT_EQ(pool->allocate(0), nullptr);

    // test too large allocation fails
    ASSERT_EQ(pool->allocate(fmt.block_size + 1), nullptr);

    struct allocation_test_data_t
    {
        size_t num_bytes = 0;
        std::unique_ptr<uint8_t[]> data = nullptr;
    };
    std::unordered_map<void *, allocation_test_data_t> pointer_map;

    constexpr size_t num_iterations = 10;
    constexpr size_t num_allocations = 12;

    // loop and create more involved allocations and checks
    for(uint32_t i = 0; i < num_iterations; ++i)
    {
        for(uint32_t j = 0; j < num_allocations; ++j)
        {
            // pow2 from 1 kB ... 4 MB
            size_t numBytes = (1024U << j);

            // check for proper upscaling to pow2
            numBytes -= j + 3;

            // actual size will be next power of two
            size_t allocatedBytes = next_pow_2(numBytes);

            auto *ptr = static_cast<uint8_t *>(pool->allocate(numBytes));
            ASSERT_NE(ptr, nullptr);

            // expect we haven't seen this pointer already
            ASSERT_EQ(pointer_map.count(ptr), 0);

            // fill memory with dummy values
            auto data = std::unique_ptr<uint8_t[]>(new uint8_t[numBytes]);
            for(uint32_t k = 0; k < numBytes; ++k){ ptr[k] = data[k] = static_cast<uint8_t>(rand() % 256); }
            pointer_map[ptr] = {numBytes, std::move(data)};

            pool_state = pool->pool_state();
            ASSERT_EQ(pool_state.allocations[allocatedBytes], i + 1);
        }
    }

    pool_state = pool->pool_state();
    ASSERT_EQ(pool_state.allocations.size(), num_allocations);
    size_t num_blocks_before_shrink = pool_state.num_blocks;

    // should do nothing (all toplevel-blocks still in use)
    pool->shrink();

    pool_state = pool->pool_state();
    ASSERT_EQ(pool_state.num_blocks, num_blocks_before_shrink);

    // sanity-check for correct content, free everything again
    for(auto &pair : pointer_map)
    {
        ASSERT_EQ(memcmp(pair.first, pair.second.data.get(), pair.second.num_bytes), 0);
        pool->free(pair.first);
    }

    // no allocations left, but still 3 unused toplevel-blocks
    pool_state = pool->pool_state();
    ASSERT_EQ(pool_state.allocations.size(), 0);
    ASSERT_EQ(pool_state.num_blocks, 3);

    // shrink to deallocate unused blocks
    pool->shrink();

    // expect an empty pool again
    pool_state = pool->pool_state();
    ASSERT_EQ(pool_state.allocations.size(), 0);
    ASSERT_EQ(pool_state.num_blocks, fmt.min_num_blocks);

    // double free -> violates assert|Expects
//    pool->free(ptr2);
}

TEST(BuddyPool, BaseclassInterface)
{
    constexpr size_t numBytes_128Mb = 1U << 27U;

    crocore::BuddyPool::create_info_t fmt;

    // 128MB
    fmt.block_size = numBytes_128Mb;
    fmt.min_block_size = 512;

    crocore::AllocatorPtr allocator = crocore::BuddyPool::create(fmt);

    struct poo_dackel_t
    {
        int foo[512] = {};
    };

    auto* dackel = new (allocator) poo_dackel_t();
    dackel->~poo_dackel_t();
    allocator->free(dackel);
}