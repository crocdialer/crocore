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

#include <atomic>
#include <future>
#include <map>
#include <mutex>
#include <chrono>
#include "crocore.hpp"

namespace crocore {

DEFINE_CLASS_PTR(Task)

/**
 * @brief   wait for completion of all tasks, represented by their futures
 * @param   tasks   the provided futures to wait for
 */
template <typename Collection>
inline void wait_all(const Collection &futures)
{
    for(const auto &f : futures)
    {
        if(f.valid()){ f.wait(); }
    }
}

class Task : public std::enable_shared_from_this<Task>
{
public:
    static TaskPtr create(const std::string &the_desc = "",
                          const std::function<void()> &the_functor = std::function<void()>())
    {
        auto task = TaskPtr(new Task());
        if(the_functor){ task->add_work(the_functor); }
        task->set_description(the_desc);
        std::unique_lock<std::mutex> scoped_lock(s_mutex);
        s_tasks[task->id()] = task;
        return task;
    }

    static std::vector<TaskPtr> current_tasks();

    static uint32_t num_tasks() { return s_tasks.size(); };

    ~Task()
    {
        auto task_str = m_description.empty() ? "task #" + std::to_string(m_id) : m_description;
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - m_start_time).count();

        std::unique_lock<std::mutex> scoped_lock(s_mutex);
        s_tasks.erase(m_id);
        LOG_DEBUG << "'" << task_str << "' completed (" << millis << " ms)";
    }

    inline uint64_t id() const { return m_id; }

    inline void set_description(const std::string &the_desc) { m_description = the_desc; }

    inline const std::string &description() const { return m_description; }

    double duration() const;

    inline void add_work(const std::function<void()> &the_work) { m_functors.push_back(the_work); }

private:
    Task() : m_id(s_id_counter++), m_start_time(std::chrono::steady_clock::now())
    {

    }

    static std::mutex s_mutex;
    static std::map<uint64_t, TaskWeakPtr> s_tasks;
    static std::atomic<uint64_t> s_id_counter;
    uint64_t m_id;
    std::chrono::steady_clock::time_point m_start_time;
    std::string m_description;
    std::vector<std::function<void()>> m_functors;
};

class ThreadPool
{
public:

    explicit ThreadPool(size_t num = 1);

    ThreadPool(ThreadPool &&other) noexcept;

    ThreadPool(const ThreadPool &other) = delete;

    ~ThreadPool();

    ThreadPool &operator=(ThreadPool other);

    /**
     * @brief   Set the number of worker-threads
     * @param   num     the desired number of threads
     */
    void set_num_threads(size_t num);

    /**
     * @return  the number of worker-threads
     */
    size_t num_threads();

    io_service_t &io_service();

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
    std::future<typename std::invoke_result<Func, Args...>::type> post(Func &&f, Args&&... args)
    {
        using result_t = typename std::invoke_result<Func, Args...>::type;
        using task_t = std::packaged_task<result_t()>;
        auto packed_task = std::make_shared<task_t>(std::bind(std::forward<Func>(f), std::forward<Args>(args)...));
        auto future = packed_task->get_future();
        post_impl(std::bind(&task_t::operator(), packed_task));
        return future;
    }

    /**
     * @brief   Manually poll the contained io_service.
     *          Necessary when this ThreadPool has no threads (is synchronous)
     *
     * @return  The number of handlers that were executed
     */
    std::size_t poll();

    /**
     * @brief   Stop execution and join all threads.
     */
    void join_all();

private:

    void post_impl(const std::function<void()> &fn);

    std::unique_ptr<struct ThreadPoolImpl> m_impl;
};
}//namespace
