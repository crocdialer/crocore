#pragma once

#include <atomic>
#include <future>
#include <deque>
#include <mutex>

#include "crocore.hpp"

namespace crocore
{

/**
 * @brief   wait for completion of all tasks, represented by their futures
 * @param   tasks   the provided futures to wait for
 */
template<typename Collection>
inline void wait_all(const Collection &futures)
{
    for(const auto &f: futures)
    {
        if(f.valid()){ f.wait(); }
    }
}

class ThreadPool
{
public:

    ThreadPool() = default;

    explicit ThreadPool(size_t num_threads){ start(num_threads); }

    ThreadPool(ThreadPool &&other) noexcept:
            ThreadPool()
    {
        swap(*this, other);
    }

    ThreadPool(const ThreadPool &other) = delete;

    ~ThreadPool(){ join_all(); }

    ThreadPool &operator=(ThreadPool other)
    {
        swap(*this, other);
        return *this;
    }

    /**
     * @brief   Set the number of worker-threads
     * @param   num     the desired number of threads
     */
    void set_num_threads(size_t num)
    {
        join_all();
        start(num);
    }

    /**
     * @return  the number of worker-threads
     */
    [[nodiscard]] size_t num_threads() const{ return m_threads.size(); }

    /**
     * @brief   post work to be processed by the ThreadPool
     *
     * @tparam  Func    function template parameter
     * @tparam  Args    template params for optional arguments
     * @param   f       the function object to execute
     * @param   args    optional params to bind to the function object
     * @return  a std::future holding the return value.
     */
    template<typename Func, typename... Args>
    std::future<typename std::invoke_result<Func, Args...>::type> post(Func &&f, Args &&... args)
    {
        using result_t = typename std::invoke_result<Func, Args...>::type;
        using packaged_task_t = std::packaged_task<result_t()>;
        auto packed_task = std::make_shared<packaged_task_t>(
                std::bind(std::forward<Func>(f), std::forward<Args>(args)...));
        auto future = packed_task->get_future();

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_queue.push_back(std::bind(&packaged_task_t::operator(), packed_task));
        }
        m_condition.notify_one();
        return future;
    }

    /**
     * @brief   Manually poll all queued tasks.
     *          useful when this ThreadPool has no threads
     *
     * @return  number of tasks processed.
     */
    std::size_t poll()
    {
        if(!m_running && m_threads.empty())
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            size_t ret = m_queue.size();

            while(!m_queue.empty())
            {
                auto task = std::move(m_queue.front());
                m_queue.pop_front();
                if(task){ task(); }
            }
            return ret;
        }
        return 0;
    }

    /**
     * @brief   Stop execution and join all threads.
     */
    void join_all()
    {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_running = false;
            m_queue.clear();
        }
        m_condition.notify_all();

        for(auto &thread: m_threads)
        {
            if(thread.joinable()){ thread.join(); }
        }
        m_threads.clear();
    }

    friend void swap(ThreadPool &lhs, ThreadPool &rhs) noexcept
    {
        std::lock(lhs.m_mutex, rhs.m_mutex);
        std::unique_lock<std::mutex> lock_lhs(lhs.m_mutex, std::adopt_lock);
        std::unique_lock<std::mutex> lock_rhs(rhs.m_mutex, std::adopt_lock);

        std::swap(lhs.m_running, rhs.m_running);
        std::swap(lhs.m_threads, rhs.m_threads);
        std::swap(lhs.m_queue, rhs.m_queue);
    }

private:

    using task_t = std::function<void()>;

    void start(size_t num_threads)
    {
        if(!num_threads){ return; }

        m_running = true;
        m_threads.resize(num_threads);

        auto worker_fn = [this]() noexcept
        {
            task_t task;

            for(;;)
            {
                {
                    std::unique_lock<std::mutex> lock(m_mutex);

                    // wait for next task
                    m_condition.wait(lock, [this]{ return !m_running || !m_queue.empty(); });

                    // exit worker if requested and nothing is left in queue
                    if(!m_running && m_queue.empty()){ return; }

                    task = std::move(m_queue.front());
                    m_queue.pop_front();
                }

                // run task
                if(task){ task(); }
            }
        };
        for(auto &thread: m_threads){ thread = std::thread(worker_fn); }
    }

    bool m_running = false;
    std::vector<std::thread> m_threads;

    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::deque<task_t> m_queue;
};
}//namespace
