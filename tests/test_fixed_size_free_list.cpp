#include <gtest/gtest.h>

#include <crocore/fixed_size_free_list.h>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

using IntPool = crocore::fixed_size_free_list<int>;

TEST(FixedSizeFreeList, CreateAndGet)
{
    IntPool pool(16, 16);
    uint32_t idx = pool.create(42);
    ASSERT_NE(idx, IntPool::s_invalid_index);
    EXPECT_EQ(pool.get(idx), 42);
    pool.destroy(idx);
}

TEST(FixedSizeFreeList, MultipleObjects)
{
    IntPool pool(32, 16);
    std::vector<uint32_t> indices;
    for(int i = 0; i < 20; ++i)
    {
        auto idx = pool.create(i * 3);
        ASSERT_NE(idx, IntPool::s_invalid_index);
        indices.push_back(idx);
    }
    for(int i = 0; i < 20; ++i) { EXPECT_EQ(pool.get(indices[i]), i * 3); }
    for(auto idx: indices) { pool.destroy(idx); }
}

TEST(FixedSizeFreeList, SlotReuseAfterDestroy)
{
    IntPool pool(16, 16);
    uint32_t first = pool.create(1);
    ASSERT_NE(first, IntPool::s_invalid_index);
    pool.destroy(first);

    uint32_t reused = pool.create(2);
    ASSERT_NE(reused, IntPool::s_invalid_index);
    EXPECT_EQ(pool.get(reused), 2);
    pool.destroy(reused);
}

TEST(FixedSizeFreeList, CapacityExhausted)
{
    IntPool pool(4, 4);
    uint32_t indices[4];
    for(int i = 0; i < 4; ++i)
    {
        indices[i] = pool.create(i);
        ASSERT_NE(indices[i], IntPool::s_invalid_index);
    }
    EXPECT_EQ(pool.create(99), IntPool::s_invalid_index);
    for(auto idx: indices) { pool.destroy(idx); }
}

TEST(FixedSizeFreeList, ReuseAfterFull)
{
    IntPool pool(4, 4);
    uint32_t indices[4];
    for(int i = 0; i < 4; ++i) { indices[i] = pool.create(i); }

    EXPECT_EQ(pool.create(99), IntPool::s_invalid_index);

    pool.destroy(indices[0]);
    uint32_t new_idx = pool.create(100);
    EXPECT_NE(new_idx, IntPool::s_invalid_index);
    EXPECT_EQ(pool.get(new_idx), 100);

    pool.destroy(new_idx);
    for(int i = 1; i < 4; ++i) { pool.destroy(indices[i]); }
}

TEST(FixedSizeFreeList, DestroyByPointer)
{
    IntPool pool(16, 16);
    uint32_t idx = pool.create(7);
    ASSERT_NE(idx, IntPool::s_invalid_index);
    pool.destroy(&pool.get(idx));
}

TEST(FixedSizeFreeList, NonTrivialType)
{
    crocore::fixed_size_free_list<std::string> pool(16, 16);
    auto idx = pool.create("hello world");
    ASSERT_NE(idx, decltype(pool)::s_invalid_index);
    EXPECT_EQ(pool.get(idx), "hello world");
    pool.destroy(idx);
}

TEST(FixedSizeFreeList, DestructorCalled)
{
    struct tracked_t
    {
        int value = 0;
        std::atomic<int> *counter = nullptr;

        tracked_t(int v, std::atomic<int> *c) : value(v), counter(c) {}

        ~tracked_t()
        {
            if(counter) { counter->fetch_add(1, std::memory_order_relaxed); }
        }
    };

    std::atomic<int> destroy_count{0};
    {
        crocore::fixed_size_free_list<tracked_t> pool(16, 16);
        auto idx1 = pool.create(1, &destroy_count);
        auto idx2 = pool.create(2, &destroy_count);

        pool.destroy(idx1);
        EXPECT_EQ(destroy_count.load(), 1);

        pool.destroy(idx2);
        EXPECT_EQ(destroy_count.load(), 2);
    }
}

TEST(FixedSizeFreeList, MultiplePagesAllocated)
{
    IntPool pool(32, 16);// 2 pages of 16
    std::vector<uint32_t> indices;
    for(int i = 0; i < 32; ++i)
    {
        auto idx = pool.create(i);
        ASSERT_NE(idx, IntPool::s_invalid_index) << "failed at i=" << i;
        indices.push_back(idx);
    }
    EXPECT_EQ(pool.create(99), IntPool::s_invalid_index);
    for(auto idx: indices) { pool.destroy(idx); }
}

TEST(FixedSizeFreeList, MoveConstruct)
{
    IntPool pool(16, 16);
    uint32_t idx = pool.create(99);
    ASSERT_NE(idx, IntPool::s_invalid_index);

    IntPool moved = std::move(pool);
    EXPECT_EQ(moved.get(idx), 99);
    moved.destroy(idx);
}

TEST(FixedSizeFreeList, MoveAssign)
{
    IntPool pool(16, 16);
    uint32_t idx = pool.create(123);
    ASSERT_NE(idx, IntPool::s_invalid_index);

    IntPool target(8, 8);
    target = std::move(pool);
    EXPECT_EQ(target.get(idx), 123);
    target.destroy(idx);
}

TEST(FixedSizeFreeList, DestroyBatch)
{
    IntPool pool(16, 16);
    auto idx0 = pool.create(0);
    auto idx1 = pool.create(1);
    auto idx2 = pool.create(2);
    ASSERT_NE(idx0, IntPool::s_invalid_index);
    ASSERT_NE(idx1, IntPool::s_invalid_index);
    ASSERT_NE(idx2, IntPool::s_invalid_index);

    IntPool::batch_t batch;
    pool.add_to_batch(batch, idx0);
    pool.add_to_batch(batch, idx1);
    pool.add_to_batch(batch, idx2);

    EXPECT_EQ(batch.m_num_objects, 3u);
    EXPECT_EQ(batch.m_first_object_index, idx0);
    EXPECT_EQ(batch.m_last_object_index, idx2);

    pool.destroy_batch(batch);

    // Freed slots are reusable
    auto new_idx = pool.create(99);
    EXPECT_NE(new_idx, IntPool::s_invalid_index);
    EXPECT_EQ(pool.get(new_idx), 99);
    pool.destroy(new_idx);
}

TEST(FixedSizeFreeList, DestroyBatchNonTrivial)
{
    std::atomic<int> destroy_count{0};
    struct tracked_t
    {
        std::atomic<int> *counter = nullptr;

        explicit tracked_t(std::atomic<int> *c) : counter(c) {}

        ~tracked_t()
        {
            if(counter) { counter->fetch_add(1, std::memory_order_relaxed); }
        }
    };

    {
        crocore::fixed_size_free_list<tracked_t> pool(16, 16);
        auto idx0 = pool.create(&destroy_count);
        auto idx1 = pool.create(&destroy_count);

        crocore::fixed_size_free_list<tracked_t>::batch_t batch;
        pool.add_to_batch(batch, idx0);
        pool.add_to_batch(batch, idx1);
        pool.destroy_batch(batch);
    }
    EXPECT_EQ(destroy_count.load(), 2);
}

TEST(FixedSizeFreeList, ConcurrentCreateDestroy)
{
    constexpr uint32_t num_threads = 8;
    constexpr uint32_t iterations = 500;
    // capacity >> num_threads so create() rarely fails
    constexpr uint32_t pool_capacity = 256;

    crocore::fixed_size_free_list<int> pool(pool_capacity, 64);

    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for(uint32_t t = 0; t < num_threads; ++t)
    {
        threads.emplace_back([&pool]() {
            for(uint32_t i = 0; i < iterations; ++i)
            {
                uint32_t idx = pool.create(static_cast<int>(i));
                if(idx != crocore::fixed_size_free_list<int>::s_invalid_index) { pool.destroy(idx); }
            }
        });
    }
    for(auto &th: threads) { th.join(); }
}
