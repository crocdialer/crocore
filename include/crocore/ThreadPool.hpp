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
