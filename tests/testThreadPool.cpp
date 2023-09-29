#include <gtest/gtest.h>
#include "crocore/crocore.hpp"
#include "crocore/ThreadPool.hpp"

std::vector<std::future<float>> schedule_work(crocore::ThreadPool &pool)
{
    std::vector<std::future<float>> async_tasks;

    for(size_t n : {6666666, 100, 1000, 100000})
    {
        auto fn = [](size_t n) -> float
        {
            float sum = 0.f;

            for(uint32_t i = 0; i < n; ++i)
            {
                sum += sqrtf(static_cast<float>(i));
            }
            return sum;
        };
        async_tasks.emplace_back(pool.post(fn, n));
    }
    return async_tasks;
}

//____________________________________________________________________________//

TEST(ThreadPool, basic)
{
    auto pool = crocore::ThreadPool(2);
    std::vector<std::future<float>> futures;

    ASSERT_TRUE(pool.num_threads() == 2);
    futures = schedule_work(pool);
    for(auto& f : futures){ ASSERT_TRUE(f.valid()); }
    crocore::wait_all(futures);

    pool.set_num_threads(4);
    ASSERT_TRUE(pool.num_threads() == 4);
    futures = schedule_work(pool);
    for(auto& f : futures){ ASSERT_TRUE(f.valid()); }
    for(auto& f : futures){ f.get(); }
}

//____________________________________________________________________________//

// EOF
