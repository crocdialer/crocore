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
        m_timingInterval(1.0),
        m_current_loop_time(1.f),
        m_running(false),
        m_main_queue(0),
        m_background_queue(4)
{
    shutdown_handler = [app = this](int){ app->set_running(false); };
    signal(SIGINT, signal_handler);

    srand(time(nullptr));
    for(int i = 0; i < argc; i++){ m_args.emplace_back(argv[i]); }

    // properties
    register_property(m_log_level);
}

int Application::run()
{
    if(m_running){ return -1; }
    m_running = true;

    // user setup-hook
    setup();

    std::chrono::steady_clock::time_point time_stamp;

    try
    {
        // main loop
        while(m_running)
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

            m_last_timestamp = time_stamp;

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
    m_num_loop_iterations++;

    double diff = double_sec_t(m_last_timestamp - m_last_measure).count();

    if(diff > m_timingInterval)
    {
        m_current_loop_time = diff / m_num_loop_iterations;
        m_num_loop_iterations = 0;
        m_last_measure = m_last_timestamp;
    }
}

void Application::update_property(const crocore::PropertyConstPtr &theProperty)
{
    if(theProperty == m_log_level){ crocore::g_logger.set_severity(crocore::Severity(m_log_level->value())); }
}

}
