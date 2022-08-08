#pragma once

#include <unordered_map>
#include <chrono>

#include <crocore/Component.hpp>
#include "crocore/ThreadPool.hpp"

namespace crocore
{

DEFINE_CLASS_PTR(Application);

class Application : public crocore::Component
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

    std::atomic<bool> loop_throttling = false;

    std::atomic<double> target_loop_frequency = 0.f;

    explicit Application(const create_info_t &create_info);

    int run();

    double application_time() const;

    /**
    * returns the current average time per loop-iteration in seconds
    */
    double current_loop_time() const{ return m_avg_loop_time; };

    /**
    * the commandline arguments provided at application start
    */
    const std::vector<std::string> &args() const{ return m_args; };

    /*!
     * this queue is processed by the main thread
     */
    crocore::ThreadPool &main_queue(){ return m_main_queue; }

    const crocore::ThreadPool &main_queue() const{ return m_main_queue; }

    /*!
    * the background queue is processed by a background threadpool
    */
    crocore::ThreadPool &background_queue(){ return m_background_queue; }

    const crocore::ThreadPool &background_queue() const{ return m_background_queue; }

    void update_property(const crocore::PropertyConstPtr &theProperty) override;

private:

    // you are supposed to implement these in a subclass
    virtual void setup() = 0;

    virtual void update(double timeDelta) = 0;

    virtual void teardown() = 0;

    virtual void poll_events() = 0;

    void update_timing();

    // timing
    size_t m_num_loop_iterations = 0;
    std::chrono::steady_clock::time_point m_start_time, m_last_timestamp, m_last_avg, m_fps_timestamp;
    double m_timing_interval = 1;

    double m_avg_loop_time;

    std::vector<std::string> m_args;

    crocore::ThreadPool m_main_queue, m_background_queue;

    // basic application properties
    crocore::Property_<uint32_t>::Ptr
            m_log_level = crocore::Property_<uint32_t>::create("log_level", (uint32_t) crocore::Severity::INFO);
};

}
