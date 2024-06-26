// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//
//  ThreadPool.hpp
//
//  Created by Fabian on 6/12/13.

#pragma once

#include <deque>
#include <future>
#include <mutex>
#include <thread>

#include "crocore.hpp"

namespace crocore
{

class ThreadPoolClassic
{
public:
    enum class Priority : uint32_t
    {
        High = 0,
        Default,
        NumPriorities
    };

    ThreadPoolClassic() = default;

    explicit ThreadPoolClassic(size_t num_threads) { start(num_threads); }

    ThreadPoolClassic(ThreadPoolClassic &&other) noexcept : ThreadPoolClassic() { swap(*this, other); }

    ThreadPoolClassic(const ThreadPoolClassic &other) = delete;

    ~ThreadPoolClassic() { join_all(); }

    ThreadPoolClassic &operator=(ThreadPoolClassic other)
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
    [[nodiscard]] size_t num_threads() const { return m_threads.size(); }

    /**
     * @brief   post work to be processed by the ThreadPool
     *
     * @tparam  Func    function template parameter
     * @tparam  Args    template params for optional arguments
     * @param   f       the function object to execute
     * @param   args    optional params to bind to the function object
     * @return  a std::future holding the return value.
     */
    template<Priority prio = Priority::Default, typename Func, typename... Args>
    std::future<typename std::invoke_result<Func, Args...>::type> post(Func &&f, Args &&...args)
    {
        using result_t = typename std::invoke_result<Func, Args...>::type;
        using packaged_task_t = std::packaged_task<result_t()>;
        auto packed_task =
                std::make_shared<packaged_task_t>(std::bind(std::forward<Func>(f), std::forward<Args>(args)...));
        auto future = packed_task->get_future();

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            constexpr uint32_t queue_index =
                    std::min(static_cast<uint32_t>(prio), static_cast<uint32_t>(Priority::Default));
            m_queues[queue_index].push_back(std::bind(&packaged_task_t::operator(), packed_task));
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
            size_t ret = 0;

            for(auto &queue: m_queues)
            {
                ret += queue.size();
                while(!queue.empty())
                {
                    auto task = std::move(queue.front());
                    queue.pop_front();
                    if(task) { task(); }
                }
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
            for(auto &queue: m_queues) { queue.clear(); }
        }
        m_condition.notify_all();

        for(auto &thread: m_threads)
        {
            if(thread.joinable()) { thread.join(); }
        }
        m_threads.clear();
    }

    friend void swap(ThreadPoolClassic &lhs, ThreadPoolClassic &rhs) noexcept
    {
        std::lock(lhs.m_mutex, rhs.m_mutex);
        std::unique_lock<std::mutex> lock_lhs(lhs.m_mutex, std::adopt_lock);
        std::unique_lock<std::mutex> lock_rhs(rhs.m_mutex, std::adopt_lock);

        std::swap(lhs.m_running, rhs.m_running);
        std::swap(lhs.m_threads, rhs.m_threads);

        for(uint32_t i = 0; i < static_cast<uint32_t>(Priority::NumPriorities); ++i)
        {
            std::swap(lhs.m_queues[i], rhs.m_queues[i]);
        }
    }

private:
    using task_t = std::function<void()>;

    void start(size_t num_threads)
    {
        if(!num_threads) { return; }

        m_running = true;
        m_threads.resize(num_threads);

        auto worker_fn = [this]() noexcept {
            task_t task;

            for(;;)
            {
                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    bool all_queues_empty = true;

                    // wait for next task
                    m_condition.wait(lock, [this, &all_queues_empty] {
                        for(const auto &queue: m_queues)
                        {
                            if(!queue.empty())
                            {
                                all_queues_empty = false;
                                break;
                            }
                        }
                        return !m_running || !all_queues_empty;
                    });

                    // exit worker if requested and nothing is left in queue
                    if(!m_running && all_queues_empty) { return; }

                    // grab task from highest prio, non-empty queue
                    for(auto &queue: m_queues)
                    {
                        if(!queue.empty())
                        {
                            task = std::move(queue.front());
                            queue.pop_front();
                            break;
                        }
                    }
                }

                // run task
                if(task) { task(); }
            }
        };
        for(auto &thread: m_threads) { thread = std::thread(worker_fn); }
    }

    bool m_running = false;
    std::vector<std::thread> m_threads;

    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::deque<task_t> m_queues[static_cast<uint32_t>(Priority::NumPriorities)];
};
}// namespace crocore
