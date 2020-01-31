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
        createInfo.minBlockSize = 512;

        // check for existence of default allocator/de-allocator (malloc/free)
        BOOST_CHECK(createInfo.allocFn);
        BOOST_CHECK(createInfo.deallocFn);

        auto pool = crocore::BuddyPool::create(createInfo);

        auto poolState = pool->state();

        // nothing was pre-allocated
        BOOST_CHECK_EQUAL(poolState.numBlocks, 0);

        // we have passed a non pow2 -> check for correct rounding to next pow2
        BOOST_CHECK_EQUAL(poolState.blockSize, numBytes_256Mb);

        // maximum height of binary tree
        BOOST_CHECK_EQUAL(poolState.maxLevel, 19);
    }

    // with pre-allocation
    {
        constexpr size_t minNumBlocks = 2;

        crocore::BuddyPool::create_info_t createInfo = {};

        // 128MB - something
        createInfo.blockSize = numBytes_128Mb - 54321;
        createInfo.minBlockSize = 2048;

        // request a minimum of blocks to be around -> pre-allocation
        createInfo.minNumBlocks = minNumBlocks;

        auto pool = crocore::BuddyPool::create(createInfo);

        auto poolState = pool->state();

        // nothing was pre-allocated
        BOOST_CHECK_EQUAL(poolState.numBlocks, minNumBlocks);

        // we have passed a non pow2 -> check for correct rounding to next pow2
        BOOST_CHECK_EQUAL(poolState.blockSize, numBytes_128Mb);

        // maximum height of binary tree
        BOOST_CHECK_EQUAL(poolState.maxLevel, 16);
    }
}


BOOST_AUTO_TEST_CASE(Allocations)
{
//    Arguments::setSystemArgs(systemArgs);
    constexpr size_t numBytes_256Mb = 1U << 28U;
    constexpr size_t numBytes_1Mb = 1U << 20U;

    crocore::BuddyPool::create_info_t fmt;

    // 256MB
    fmt.blockSize = numBytes_256Mb;
    fmt.minBlockSize = 512;

    // no preallocation, pool will allocate on first request
    fmt.minNumBlocks = 0;

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

    struct allocation_test_data_t
    {
        size_t num_bytes = 0;
        std::unique_ptr<uint8_t[]> data = nullptr;
    };
    std::unordered_map<void *, allocation_test_data_t> pointer_map;

    // loop and create more involved allocations and checks
    for(uint32_t i = 0; i < 10; ++i)
    {
        // pow2 from 1 kB ... 1 MB
        size_t numBytes = (1024U << i);

        // check for proper upscaling to pow2
        numBytes -= i + 3;

        // actual size will be next power of two
        size_t allocatedBytes = next_pow_2(numBytes);
        constexpr size_t num_allocations = 100;

        for(uint32_t j = 0; j < num_allocations; ++j)
        {
            uint8_t *ptr = static_cast<uint8_t *>(pool->allocate(numBytes));
            BOOST_CHECK_NE(ptr, nullptr);

            // expect we haven't seen this pointer already
            BOOST_CHECK_EQUAL(pointer_map.count(ptr), 0);

            // fill memory with dummy values
            auto data = std::unique_ptr<uint8_t[]>(new uint8_t[numBytes]);

            for(uint32_t k = 0; k < numBytes; ++k)
            {
                uint8_t rnd_value = static_cast<uint8_t>(rand() % 255);
                ptr[k] = data[k] = rnd_value;
            }
            pointer_map[ptr] = {numBytes, std::move(data)};
        }
        poolState = pool->state();

        BOOST_CHECK_EQUAL(poolState.allocations.size(), i + 1);
        BOOST_CHECK_EQUAL(poolState.allocations[allocatedBytes], num_allocations);
    }

    // sanity-check for correct content, free everything again
    for(auto &pair : pointer_map)
    {
        BOOST_CHECK_EQUAL(memcmp(pair.first, pair.second.data.get(), pair.second.num_bytes), 0);
        pool->free(pair.first);
    }

    // expect an empty pool again
    poolState = pool->state();
    BOOST_CHECK_EQUAL(poolState.allocations.size(), 0);

    // double free -> violates assert|Expects
//    pool->free(ptr2);
}
