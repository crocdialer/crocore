#pragma once

#include <unordered_map>
#include <chrono>

#include <crocore/ThreadPool.hpp>
#include <crocore/precise_sleep.hpp>

namespace crocore
{

DEFINE_CLASS_PTR(Application)

class Application : public std::enable_shared_from_this<Application>
{
public:

    struct create_info_t
    {
        bool loop_throttling = false;
        float target_loop_frequency = 0.f;
        std::vector<std::string> arguments;
        uint32_t num_background_threads = std::max(1U, std::thread::hardware_concurrency());
    };

    std::atomic<bool> running = false;

    std::atomic<int> return_type = EXIT_SUCCESS;

    std::atomic<bool> loop_throttling = false;

    std::atomic<double> target_loop_frequency = 0.f;

    explicit Application(const create_info_t &create_info);

    virtual ~Application() = default;

    int run();

    const std::string &name() const{ return m_name; };

    [[nodiscard]] double application_time() const;

    /**
    * returns the current average time per loop-iteration in seconds
    */
    [[nodiscard]] double current_loop_time() const{ return m_avg_loop_time; };

    /**
    * the commandline arguments provided at application start
    */
    [[nodiscard]] const std::vector<std::string> &args() const{ return m_args; };

    /*!
     * this queue is processed by the main thread
     */
    crocore::ThreadPool &main_queue(){ return m_main_queue; }

    [[nodiscard]] const crocore::ThreadPool &main_queue() const{ return m_main_queue; }

    /*!
    * the background queue is processed by a background threadpool
    */
    crocore::ThreadPool &background_queue(){ return m_background_queue; }

    [[nodiscard]] const crocore::ThreadPool &background_queue() const{ return m_background_queue; }

private:

    // you are supposed to implement these in a subclass
    virtual void setup() = 0;

    virtual void update(double timeDelta) = 0;

    virtual void teardown() = 0;

    virtual void poll_events() = 0;

    void update_timing();

    std::string m_name;

    // timing
    size_t m_num_loop_iterations = 0;
    std::chrono::steady_clock::time_point m_start_time, m_last_timestamp, m_last_avg, m_fps_timestamp;
    double m_timing_interval = 1;

    double m_avg_loop_time;

    std::vector<std::string> m_args;

    crocore::ThreadPool m_main_queue, m_background_queue;

    crocore::precise_sleep m_precise_sleep;
};

}
