#include <gtest/gtest.h>
#include <crocore/MemoryCache.hpp>
#include <unordered_map>

TEST(MemoryCache, Constructors)
{
    // nothing fancy here
    {
        crocore::MemoryCache::create_info_t createInfo;

        // 4KB minimum allocations
        createInfo.min_size = 1U << 12U;

        // check for existence of default allocator/de-allocator (malloc/free)
        ASSERT_TRUE(createInfo.alloc_fn);
        ASSERT_TRUE(createInfo.dealloc_fn);

        auto cache = crocore::MemoryCache::create(createInfo);

        ASSERT_NE(cache, nullptr);
    }
}

TEST(MemoryCache, Allocations)
{
    crocore::MemoryCache::create_info_t create_info;

    // 4KB minimum allocations
    create_info.min_size = 1U << 12U;

    // tolerate big enough chunks up to 2x the requested size
    create_info.size_tolerance = 2.f;

    auto cache = crocore::MemoryCache::create(create_info);

    ASSERT_NE(cache, nullptr);

    constexpr size_t num_bytes_32Mb = 1U << 25U;
    constexpr size_t num_bytes_16Mb = 1U << 24U;
    constexpr size_t num_bytes_1Mb = 1U << 20U;

    // allocate first block
    void* ptr1 = cache->allocate(num_bytes_32Mb);
    ASSERT_NE(ptr1, nullptr);

    // free again
    cache->free(ptr1);

    // 1MB allocation
    void* ptr2 = cache->allocate(num_bytes_1Mb);

    // allocation succeeded
    ASSERT_NE(ptr2, nullptr);

    ASSERT_EQ(cache->state().num_allocations, 2);
    ASSERT_EQ(cache->state().num_bytes_allocated, num_bytes_32Mb + num_bytes_1Mb);
    ASSERT_EQ(cache->state().num_bytes_used, num_bytes_1Mb);

    // should have been assigned different chunks
    // -> 32MB chunk is too big and was not re-used here
    ASSERT_NE(ptr1, ptr2);

    void* ptr3 = cache->allocate(num_bytes_16Mb);
    ASSERT_NE(ptr3, nullptr);

    // corner case, 32MB chunk should have been re-used
    ASSERT_EQ(ptr1, ptr3);

    // free once again
    cache->free(ptr3);

    // request only one byte less
    ptr3 = cache->allocate(num_bytes_16Mb - 1);

    // corner case, 32MB chunk should NOT have been re-used
    ASSERT_NE(ptr1, ptr3);

    // check minimum size
    void *ptr4 = cache->allocate(1);
    cache->free(ptr4);
    void *ptr5 = cache->allocate(create_info.min_size);

    // 1 byte and min_size allocation end up being the same
    ASSERT_EQ(ptr4, ptr5);

    // free once again
    cache->free(ptr3);

    ASSERT_EQ(cache->state().num_allocations, 4);

    // frees unused chunks
    cache->shrink();

    ASSERT_EQ(cache->state().num_allocations, 2);
}

//! test leak-free destruction via baseclass-pointer
TEST(MemoryCache, BaseClassPointer)
{
    crocore::MemoryCache::create_info_t create_info;

    // same as default
    create_info.alloc_fn = ::malloc;

    // dealloc will signal a flag
    bool flag = false;
    create_info.dealloc_fn = [&flag](void *p){ ::free(p); flag = true; };

    // hold the cache via baseclass-pointer
    crocore::AllocatorPtr allocator = crocore::MemoryCache::create(create_info);

    // allocate something
    ASSERT_NE(allocator->allocate(42), nullptr);

    // MemoryCache reset via baseclass-pointer, still held some memory
    allocator.reset();

    // flag should have been set -> virtual destruction worked + no leak
    ASSERT_TRUE(flag);
}
