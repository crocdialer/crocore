#pragma once

#include <atomic>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <semaphore>

#include "fixed_size_free_list.h"
#include "utils.hpp"

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
        if(f.valid()) { f.wait(); }
    }
}

template<uint32_t QUEUE_SIZE = 2048>
class ThreadPool_
{
public:
    ThreadPool_() = default;

    explicit ThreadPool_(size_t num_threads) { start(num_threads); }

    ThreadPool_(ThreadPool_ &&other) noexcept : ThreadPool_() { swap(*this, other); }

    ThreadPool_(const ThreadPool_ &other) = delete;

    ~ThreadPool_() { join_all(); }

    ThreadPool_ &operator=(ThreadPool_ other)
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
    template<typename Func, typename... Args>
    std::future<typename std::invoke_result<Func, Args...>::type> post(Func &&f, Args &&...args)
    {
        using result_t = typename std::invoke_result<Func, Args...>::type;
        using packaged_task_t = std::packaged_task<result_t()>;
        auto packed_task =
                std::make_shared<packaged_task_t>(std::bind(std::forward<Func>(f), std::forward<Args>(args)...));
        auto future = packed_task->get_future();

        // loop until we get a task from the free list
        uint32_t index;
        for(;;)
        {
            index = m_tasks.create();
            if(index != fixed_size_free_list<task_t>::s_invalid_index) { break; }

            // No jobs available
            assert(false);
        }
        auto &t = m_tasks.get(index);
        t = std::bind(&packaged_task_t::operator(), packed_task);
        queue_task(&t);
        m_semaphore.release();
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
        size_t ret = 0;
        if(!m_running && m_threads.empty())
        {
            for(uint32_t head = 0; head != m_tail; ++head)
            {
                // Fetch job
                task_t *job_ptr = m_queue[head & (QUEUE_SIZE - 1)].exchange(nullptr);
                if(job_ptr)
                {
                    if(*job_ptr) { (*job_ptr)(); }
                    m_tasks.destroy(job_ptr);
                    ret++;
                }
            }
        }
        return ret;
    }

    /**
     * @brief   Stop execution and join all threads.
     */
    void join_all()
    {
        // signal threads that we want to stop and wake them up
        m_running = false;
        m_semaphore.release((int32_t) m_threads.size());

        for(auto &thread: m_threads)
        {
            if(thread.joinable()) { thread.join(); }
        }
        m_threads.clear();

        poll();

        // destroy heads and reset tail
        m_heads.reset();
        m_tail = 0;
    }

    friend void swap(ThreadPool_ &lhs, ThreadPool_ &rhs) noexcept
    {
        std::lock(lhs.m_mutex, rhs.m_mutex);
        std::unique_lock<std::mutex> lock_lhs(lhs.m_mutex, std::adopt_lock);
        std::unique_lock<std::mutex> lock_rhs(rhs.m_mutex, std::adopt_lock);

        std::swap(lhs.m_threads, rhs.m_threads);
        std::swap(lhs.m_tasks, rhs.m_tasks);
        for(uint32_t i = 0; i < QUEUE_SIZE; ++i) { rhs.m_queue[i] = lhs.m_queue[i].exchange(rhs.m_queue[i]); }
        std::swap(lhs.m_heads, rhs.m_heads);
        rhs.m_tail = lhs.m_tail.exchange(rhs.m_tail);
        rhs.m_running = lhs.m_running.exchange(rhs.m_running);

        // TODO: semaphore
    }

private:
    using task_t = std::function<void()>;

    void start(size_t num_threads)
    {
        constexpr uint32_t max_num_tasks = 1024;
        m_tasks = fixed_size_free_list<task_t>(max_num_tasks, max_num_tasks);

        if(!num_threads) { return; }
        m_threads.resize(num_threads);

        for(auto &t: m_queue) { t = nullptr; }
        m_running = true;

        // allocate heads
        m_heads = std::make_unique<std::atomic<uint32_t>[]>(num_threads);
        for(uint32_t i = 0; i < num_threads; ++i) { m_heads[i] = 0; }

        auto worker_fn = [this](uint32_t thread_idx) noexcept {
            auto &head = m_heads[thread_idx];

            while(m_running)
            {
                // Wait for jobs
                m_semaphore.acquire();

                // Loop over the queue
                while(head != m_tail)
                {
                    // Exchange any job pointer we find with a nullptr
                    std::atomic<task_t *> &task = m_queue[head & (QUEUE_SIZE - 1)];

                    if(task.load() != nullptr)
                    {
                        task_t *task_ptr = task.exchange(nullptr);
                        if(task_ptr)
                        {
                            if(*task_ptr) { (*task_ptr)(); }
                            m_tasks.destroy(task_ptr);
                        }
                    }
                    head++;
                }
            }
        };

        for(uint32_t i = 0; i < num_threads; ++i)
        {
            m_threads[i] = std::thread([worker_fn, i] { worker_fn(i); });
        }
    }

    [[nodiscard]] uint32_t get_head() const
    {
        // find minimal value across all threads
        uint32_t head = m_tail;
        for(size_t i = 0; i < m_threads.size(); ++i) { head = std::min(head, m_heads[i].load()); }
        return head;
    }

    void queue_task(task_t *t)
    {
        // Need to read head first because otherwise the tail can already have passed the head
        // We read the head outside of the loop since it involves iterating over all threads and we only need to update
        // it if there's not enough space in the queue.
        uint32_t head = get_head();

        for(;;)
        {
            // Check if there's space in the queue
            uint32_t old_value = m_tail;

            if(old_value - head >= QUEUE_SIZE)
            {
                // We calculated the head outside of the loop, update head (and we also need to update tail to prevent it from passing head)
                head = get_head();
                old_value = m_tail;

                // Second check if there's space in the queue
                if(old_value - head >= QUEUE_SIZE)
                {
                    // Wake up all threads in order to ensure that they can clear any nullptrs they may not have processed yet
                    m_semaphore.release((uint32_t) m_threads.size());

                    // Sleep a little (we have to wait for other threads to update their head pointer in order for us to be able to continue)
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                    continue;
                }
            }

            // Write the job pointer if the slot is empty
            task_t *expected_job = nullptr;
            bool success = m_queue[old_value & (QUEUE_SIZE - 1)].compare_exchange_strong(expected_job, t);

            // Regardless of who wrote the slot, we will update the tail (if the successful thread got scheduled out
            // after writing the pointer we still want to be able to continue)
            m_tail.compare_exchange_strong(old_value, old_value + 1);

            // If we successfully added our job we're done
            if(success) break;
        }
    }

    std::vector<std::thread> m_threads;

    // job queue
    fixed_size_free_list<task_t> m_tasks;
    std::atomic<task_t *> m_queue[QUEUE_SIZE];

    // Head and tail of the queue, do this value modulo s_max_queue_size - 1 to get the element in the m_queue array

    // per executing thread the head of the current queue
    std::unique_ptr<std::atomic<uint32_t>[]> m_heads = nullptr;

    // tail (write end) of the queue
    /*alignas(k_cache_line_size) */
    std::atomic<uint32_t> m_tail = 0;

    // semaphore used to signal worker threads
    std::counting_semaphore<32> m_semaphore{0};
    std::atomic<bool> m_running = false;
};

using ThreadPool = ThreadPool_<2048>;

}// namespace crocore
