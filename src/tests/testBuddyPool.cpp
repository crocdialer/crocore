#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>

#include <crocore/BuddyPool.hpp>
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


BOOST_AUTO_TEST_CASE(Constructors)
{
    constexpr size_t numBytes_256Mb = 1U << 28U;
    constexpr size_t numBytes_128Mb = numBytes_256Mb / 2;

    // no pre-allocation
    {
        crocore::BuddyPool::create_info_t createInfo;

        // 256MB - something
        createInfo.blockSize = numBytes_256Mb - 12345;
        createInfo.min_block_size = 512;

        // check for existence of default allocator/de-allocator (malloc/free)
        BOOST_CHECK(createInfo.alloc_fn);
        BOOST_CHECK(createInfo.dealloc_fn);

        auto pool = crocore::BuddyPool::create(createInfo);

        auto poolState = pool->state();

        // nothing was pre-allocated
        BOOST_CHECK_EQUAL(poolState.num_blocks, 0);

        // we have passed a non pow2 -> check for correct rounding to next pow2
        BOOST_CHECK_EQUAL(poolState.block_size, numBytes_256Mb);

        // maximum height of binary tree
        BOOST_CHECK_EQUAL(poolState.max_level, 19);
    }

    // with pre-allocation
    {
        constexpr size_t minNumBlocks = 2;

        crocore::BuddyPool::create_info_t createInfo = {};

        // 128MB - something
        createInfo.blockSize = numBytes_128Mb - 54321;
        createInfo.min_block_size = 2048;

        // request a minimum of blocks to be around -> pre-allocation
        createInfo.min_num_blocks = minNumBlocks;

        auto pool = crocore::BuddyPool::create(createInfo);

        auto poolState = pool->state();

        // check for pre-allocated blocks
        BOOST_CHECK_EQUAL(poolState.num_blocks, minNumBlocks);

        // we have passed a non pow2 -> check for correct rounding to next pow2
        BOOST_CHECK_EQUAL(poolState.block_size, numBytes_128Mb);

        // maximum height of binary tree
        BOOST_CHECK_EQUAL(poolState.max_level, 16);
    }
}


BOOST_AUTO_TEST_CASE(Allocations)
{
    constexpr size_t numBytes_256Mb = 1U << 28U;
    constexpr size_t numBytes_1Mb = 1U << 20U;

    crocore::BuddyPool::create_info_t fmt;

    // 256MB
    fmt.blockSize = numBytes_256Mb;
    fmt.min_block_size = 512;

    // no preallocation, pool will allocate on first request
    fmt.min_num_blocks = 0;

    auto pool = crocore::BuddyPool::create(fmt);

    // allocate entire toplevel block
    void *ptr1 = pool->allocate(numBytes_256Mb);
    BOOST_CHECK_NE(ptr1, nullptr);

    // free pointer
    pool->free(ptr1);

    // 1MB allocation
    void *ptr2 = pool->allocate(numBytes_1Mb);

    // allocation succeeded
    BOOST_CHECK_NE(ptr2, nullptr);

    // should have been assigned the same offset like before
    BOOST_CHECK_EQUAL(ptr1, ptr2);

    auto poolState = pool->state();

    // expect exactly one 1MB-block here
    BOOST_CHECK_EQUAL(poolState.allocations.size(), 1);
    BOOST_CHECK_EQUAL(poolState.allocations[numBytes_1Mb], 1);

    // free pointer
    pool->free(ptr2);

    // expect an empty pool
    poolState = pool->state();
    BOOST_CHECK_EQUAL(poolState.allocations.size(), 0);

    // test zero byte allocation fails
    BOOST_CHECK_EQUAL(pool->allocate(0), nullptr);

    // test too large allocation fails
    BOOST_CHECK_EQUAL(pool->allocate(fmt.blockSize + 1), nullptr);

    struct allocation_test_data_t
    {
        size_t num_bytes = 0;
        std::unique_ptr<uint8_t[]> data = nullptr;
    };
    std::unordered_map<void *, allocation_test_data_t> pointer_map;

    constexpr size_t num_iterations = 100;
    constexpr size_t num_allocations = 10;

    // loop and create more involved allocations and checks
    for(uint32_t i = 0; i < num_iterations; ++i)
    {
        for(uint32_t j = 0; j < num_allocations; ++j)
        {
            // pow2 from 1 kB ... 1 MB
            size_t numBytes = (1024U << j);

            // check for proper upscaling to pow2
            numBytes -= j + 3;

            // actual size will be next power of two
            size_t allocatedBytes = next_pow_2(numBytes);

            uint8_t *ptr = static_cast<uint8_t *>(pool->allocate(numBytes));
            BOOST_CHECK_NE(ptr, nullptr);

            // expect we haven't seen this pointer already
            BOOST_CHECK_EQUAL(pointer_map.count(ptr), 0);

            // fill memory with dummy values
            auto data = std::unique_ptr<uint8_t[]>(new uint8_t[numBytes]);
            for(uint32_t k = 0; k < numBytes; ++k){ ptr[k] = data[k] = static_cast<uint8_t>(rand() % 256); }
            pointer_map[ptr] = {numBytes, std::move(data)};

            poolState = pool->state();
            BOOST_CHECK_EQUAL(poolState.allocations[allocatedBytes], i + 1);
        }
    }

    poolState = pool->state();
    BOOST_CHECK_EQUAL(poolState.allocations.size(), num_allocations);

    // sanity-check for correct content, free everything again
    for(auto &pair : pointer_map)
    {
        BOOST_CHECK_EQUAL(memcmp(pair.first, pair.second.data.get(), pair.second.num_bytes), 0);
        pool->free(pair.first);
    }

    // expect an empty pool again
    poolState = pool->state();
    BOOST_CHECK_EQUAL(poolState.allocations.size(), 0);
    BOOST_CHECK_EQUAL(poolState.num_blocks, fmt.min_num_blocks);

    // double free -> violates assert|Expects
//    pool->free(ptr2);
}
