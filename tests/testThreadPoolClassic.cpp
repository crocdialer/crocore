#include "crocore/ThreadPoolClassic.hpp"
#include "crocore/crocore.hpp"
#include <gtest/gtest.h>

template<typename R>
bool is_ready(std::future<R> const &f)
{
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

template<crocore::ThreadPoolClassic::Priority prio = crocore::ThreadPoolClassic::Priority::Default>
std::vector<std::future<float>> schedule_work(crocore::ThreadPoolClassic &pool)
{
    std::vector<std::future<float>> async_tasks;

    for(size_t n: {6666666, 100, 1000, 100000})
    {
        auto fn = [](size_t n) -> float {
            float sum = 0.f;

            for(uint32_t i = 0; i < n; ++i) { sum += sqrtf(static_cast<float>(i)); }
            return sum;
        };
        async_tasks.emplace_back(pool.post<prio>(fn, n));
    }
    return async_tasks;
}

//____________________________________________________________________________//

TEST(ThreadPoolClassic, basic)
{
    // assign/move-ctor
    auto pool = crocore::ThreadPoolClassic(2);
    std::vector<std::future<float>> futures;

    ASSERT_TRUE(pool.num_threads() == 2);
    futures = schedule_work(pool);
    for(auto &f: futures) { ASSERT_TRUE(f.valid()); }
    crocore::wait_all(futures);

    pool.set_num_threads(4);
    ASSERT_TRUE(pool.num_threads() == 4);
    futures = schedule_work(pool);
    for(auto &f: futures) { ASSERT_TRUE(f.valid()); }
    for(auto &f: futures) { f.get(); }
}

//____________________________________________________________________________//

TEST(ThreadPoolClassic, priorities)
{
    auto pool = crocore::ThreadPoolClassic(2);
    auto high_prio_futures = schedule_work<crocore::ThreadPoolClassic::Priority::High>(pool);
    auto default_prio_futures = schedule_work<crocore::ThreadPoolClassic::Priority::Default>(pool);

    for(auto &f: high_prio_futures) { ASSERT_TRUE(f.valid()); }
    for(auto &f: default_prio_futures) { ASSERT_TRUE(f.valid()); }
    crocore::wait_all(high_prio_futures);
}

//____________________________________________________________________________//

TEST(ThreadPool, polling)
{
    // default ctor does not init any threads, but we can post work and poll
    crocore::ThreadPoolClassic pool;
    auto futures = schedule_work(pool);
    pool.poll();
    for(auto &f: futures) { ASSERT_TRUE(f.valid()); }
    for(auto &f: futures) { f.get(); }
}

// EOF
