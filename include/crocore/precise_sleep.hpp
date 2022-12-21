//
// Created by crocdialer on 21.12.22.
//

#pragma once

#include <chrono>
#include <thread>
#include <cmath>

namespace crocore
{

/**
 * @brief   precise_sleep is a function-object and can be used to sleep for a certain duration with high precision.
 *
 * @see     https://blat-blatnik.github.io/computerBear/making-accurate-sleep-function/
 */
class precise_sleep
{
public:

    template<typename Rep, typename Period>
    inline void operator()(const std::chrono::duration<Rep, Period> &duration)
    {
        double seconds = std::chrono::duration_cast<double_sec_t>(duration).count();

        while(seconds > estimate)
        {
            auto start = std::chrono::high_resolution_clock::now();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            auto end = std::chrono::high_resolution_clock::now();

            double observed = std::chrono::duration_cast<double_sec_t>(end - start).count();
            seconds -= observed;

            // increment, handle overflow
            count = std::max<uint64_t>(count + 1, 2);

            auto delta = observed - mean;
            mean += delta / static_cast<double>(count);
            m2 += delta * (observed - mean);
            double stddev = std::sqrt(m2 / static_cast<double>(count - 1));
            estimate = mean + stddev;
        }

        // spin lock
        auto start = std::chrono::high_resolution_clock::now();
        while(double_sec_t(std::chrono::high_resolution_clock::now() - start).count() < seconds){}
    }

private:

    using double_sec_t = std::chrono::duration<double, std::chrono::seconds::period>;

    double estimate = 5e-3;
    double mean = 5e-3;
    double m2 = 0;
    uint64_t count = 1;
};

}
