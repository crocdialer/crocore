//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test

// each test module could contain no more then one 'main' file with init function defined
// alternatively you could define init function yourself
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
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
                sum += sqrtf(i);
            }
            return sum;
        };
        async_tasks.emplace_back(pool.post(fn, n));
    }

    // check if all futures are valid
    for(auto& future : async_tasks){ BOOST_CHECK(future.valid()); }
    return async_tasks;
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( testThreadPool )
{
    auto pool = crocore::ThreadPool(2);
    std::vector<std::future<float>> futures;

    BOOST_CHECK(pool.num_threads() == 2);
    futures = schedule_work(pool);
    crocore::wait_all(futures);

    pool.set_num_threads(4);
    BOOST_CHECK(pool.num_threads() == 4);
    futures = schedule_work(pool);
    for(auto& f : futures){ f.get(); }
}

//____________________________________________________________________________//

// EOF
