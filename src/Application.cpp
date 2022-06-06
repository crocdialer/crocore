//
// Created by crocdialer on 4/13/19.
//

#include <signal.h>

#include "crocore/filesystem.hpp"
#include "crocore/Application.hpp"

namespace crocore
{

// 1 double per second
using double_sec_t = std::chrono::duration<double, std::chrono::seconds::period>;

namespace
{
std::function<void(int)> shutdown_handler;

void signal_handler(int signal){ if(shutdown_handler){ shutdown_handler(signal); }}
} // namespace

Application::Application(int argc, char *argv[]) :
        Component(argc ? crocore::fs::get_filename_part(argv[0]) : "vierkant_app"),
        m_start_time(std::chrono::steady_clock::now()),
        m_last_timestamp(std::chrono::steady_clock::now()),
        m_timing_interval(1.0),
        m_avg_loop_time(1.f),
        m_main_queue(0),
        m_background_queue(std::max(1U, std::thread::hardware_concurrency()))
{
    shutdown_handler = [app = this](int){ app->running = false; };
    signal(SIGINT, signal_handler);

    srand(time(nullptr));
    for(int i = 0; i < argc; i++){ m_args.emplace_back(argv[i]); }

    // properties
    register_property(m_log_level);
}

int Application::run()
{
    if(running){ return -1; }
    running = true;

    // user setup-hook
    setup();

    std::chrono::steady_clock::time_point time_stamp;

    try
    {
        // main loop
        while(running)
        {
            // get current time
            time_stamp = std::chrono::steady_clock::now();

            // poll io_service if no separate worker-threads exist
            if(!m_main_queue.num_threads()) m_main_queue.poll();

            // poll input events
            poll_events();

            // time elapsed since last frame
            double time_delta = double_sec_t(time_stamp - m_last_timestamp).count();

            // call update callback
            update(time_delta);

            // perform fps-timing
            update_timing();
        }

        // manage teardown, save stuff etc.
        teardown();

    } catch(std::exception &e)
    {
        LOG_ERROR << e.what();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

double Application::application_time() const
{
    auto current_time = std::chrono::steady_clock::now();
    return double_sec_t(current_time - m_start_time).count();
}

void Application::update_timing()
{
    auto now = std::chrono::steady_clock::now();
    uint32_t frame_us = std::chrono::duration_cast<std::chrono::microseconds>(
            now - m_last_timestamp).count();

    m_num_loop_iterations++;
    double diff = double_sec_t(now - m_last_measure).count();

    if(diff > m_timing_interval)
    {
        m_avg_loop_time = diff / static_cast<double>(m_num_loop_iterations);
        m_num_loop_iterations = 0;
        m_last_measure = now;
    }

    // fps throttling
    double fps = target_fps;

    if(fps > 0)
    {
        const uint32_t desired_frametime_us = std::ceil(1.0e6 / fps);

        if (frame_us < desired_frametime_us)
        {
            uint32_t sleep_us = desired_frametime_us - frame_us;
            std::this_thread::sleep_for(std::chrono::microseconds(sleep_us));
        }

//    spdlog::trace("frame: {} us -- target-fps: {} Hz -- sleeping: {} us", frame_us, fps, sleep_us);
    }
    m_last_timestamp = std::chrono::steady_clock::now();

}

void Application::update_property(const crocore::PropertyConstPtr &theProperty)
{
    if(theProperty == m_log_level){ crocore::g_logger.set_severity(crocore::Severity(m_log_level->value())); }
}

}
