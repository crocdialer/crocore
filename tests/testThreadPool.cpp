#include "crocore/ThreadPool.hpp"
#include "crocore/crocore.hpp"
#include <gtest/gtest.h>

std::vector<std::future<float>> schedule_work(crocore::ThreadPool &pool, bool track)
{
    std::vector<std::future<float>> async_tasks;

    for(size_t n: {6666666, 100, 1000, 100000})
    {
        auto fn = [](size_t n) -> float {
            float sum = 0.f;

            for(uint32_t i = 0; i < n; ++i) { sum += sqrtf(static_cast<float>(i)); }
            return sum;
        };
        if(track) { async_tasks.emplace_back(pool.post(fn, n)); }
        else { pool.post_no_track(fn, n); }
    }
    return async_tasks;
}

//____________________________________________________________________________//

TEST(ThreadPool, basic)
{
    // assign/move-ctor
    auto pool = crocore::ThreadPool(2);
    std::vector<std::future<float>> futures;

    ASSERT_TRUE(pool.num_threads() == 2);
    futures = schedule_work(pool, true);
    for(auto &f: futures) { ASSERT_TRUE(f.valid()); }
    crocore::wait_all(futures);

    pool.set_num_threads(4);
    ASSERT_TRUE(pool.num_threads() == 4);
    futures = schedule_work(pool, true);
    for(auto &f: futures) { ASSERT_TRUE(f.valid()); }
    for(auto &f: futures) { f.get(); }
}

//____________________________________________________________________________//

TEST(ThreadPool, post_no_track)
{
    auto pool = crocore::ThreadPool(2);
    schedule_work(pool, false);
}

TEST(ThreadPool, polling)
{
    // default ctor does not init any threads, but we can post work and poll
    crocore::ThreadPool pool;
    auto futures = schedule_work(pool, true);
    pool.poll();
    for(auto &f: futures) { ASSERT_TRUE(f.valid()); }
    for(auto &f: futures) { f.get(); }
}

// EOF
