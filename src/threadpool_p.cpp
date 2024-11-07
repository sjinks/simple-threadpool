#include "threadpool_p.h"
#include "threadpool.h"

#include <algorithm>
#include <exception>
#include <utility>

namespace {

template<typename T>
inline T atomic_fetch_max(std::atomic<T>& atomic_var, T new_value)
{
    T old_value = atomic_var.load();

    while (old_value < new_value && !atomic_var.compare_exchange_weak(old_value, new_value)) {
        // Do nothing
    }

    return old_value;
}

void default_after_work(bool)
{
    // Do nothing
}

}  // namespace
namespace wwa {

using unique_lock = std::unique_lock<std::mutex>;

thread_pool_private::thread_pool_private(std::size_t n)
    : m_num_threads((n == 0) ? std::thread::hardware_concurrency() : n)
{
    this->m_threads.reserve(this->m_num_threads);
    this->m_stop_sources.resize(this->m_num_threads);
    for (std::size_t i = 0; i < this->m_num_threads; ++i) {
        this->m_threads.emplace_back(worker_thread, this, i);
    }
}

thread_pool_private::~thread_pool_private()
{
    for (auto& thread : this->m_threads) {
        thread.request_stop();
    }

    const std::scoped_lock<std::mutex> lock(this->m_mutex);
    for (const auto& item : this->m_work_queue) {
        item->stop_source.request_stop();
        item->after_work(true);
    }

    // GNU libstdc++ declares `std::stop_source.request_stop()` as `const`
    // According to https://en.cppreference.com/w/cpp/thread/stop_source/request_stop,
    // it is not `const`.
    for (auto& stop_source : this->m_stop_sources) {
        stop_source.request_stop();
    }

    this->m_work_queue.clear();
    this->m_cv.notify_all();
}

thread_pool::task_t
thread_pool_private::submit(const thread_pool::worker_t& worker, const thread_pool::after_work_t& after_work)
{
    this->m_tasks_queued.fetch_add(1U, std::memory_order_relaxed);
    const std::scoped_lock<std::mutex> lock(this->m_mutex);
    const auto& item = this->m_work_queue.emplace_back(
        std::make_shared<work_item>(worker, after_work ? after_work : default_after_work)
    );
    this->m_cv.notify_one();
    return item;
}

bool thread_pool_private::cancel(const thread_pool::task_t& task)
{
    auto sp_task = task.lock();
    if (!sp_task) {
        return false;
    }

    sp_task->stop();

    auto predicate = [&sp_task](const std::shared_ptr<work_item>& item) { return item == sp_task; };

    const std::scoped_lock<std::mutex> lock(this->m_mutex);
    if (auto it = std::ranges::find_if(this->m_work_queue, predicate); it != this->m_work_queue.end()) {
        this->m_work_queue.erase(it);
        this->m_tasks_canceled.fetch_add(1U, std::memory_order_relaxed);
        return true;
    }

    return false;
}

void thread_pool_private::wait()
{
    unique_lock lock(this->m_mutex);
    this->m_drained_cv.wait(lock, [this] { return this->m_work_queue.empty() && this->m_active_threads == 0; });
}

bool thread_pool_private::wait_until(const std::chrono::time_point<std::chrono::steady_clock>& abs_time)
{
    unique_lock lock(this->m_mutex);
    return this->m_drained_cv.wait_until(lock, abs_time, [this] {
        return this->m_work_queue.empty() && this->m_active_threads == 0;
    });
}

std::size_t thread_pool_private::num_threads() const noexcept
{
    return this->m_num_threads;
}

std::size_t thread_pool_private::active_threads() const noexcept
{
    return this->m_active_threads;
}

std::size_t thread_pool_private::max_active_threads() const noexcept
{
    return this->m_max_active_threads;
}

std::size_t thread_pool_private::work_queue_size() const
{
    const std::scoped_lock<std::mutex> lock(this->m_mutex);
    return this->m_work_queue.size();
}

std::size_t thread_pool_private::tasks_queued() const noexcept
{
    return this->m_tasks_queued;
}

std::size_t thread_pool_private::tasks_completed() const noexcept
{
    return this->m_tasks_completed;
}

std::size_t thread_pool_private::tasks_failed() const noexcept
{
    return this->m_tasks_failed;
}

std::size_t thread_pool_private::tasks_canceled() const noexcept
{
    return this->m_tasks_canceled;
}

void thread_pool_private::worker_thread(
    const std::stop_token& stop_token, thread_pool_private* pool, std::size_t thread_index
)
{
    while (!stop_token.stop_requested()) {
        unique_lock lock(pool->m_mutex);
        pool->m_cv.wait(lock, stop_token, [&pool] { return !pool->m_work_queue.empty(); });

        if (!stop_token.stop_requested()) {
            auto task = std::move(pool->m_work_queue.front());
            pool->m_work_queue.pop_front();

            if (!task->stop_source.stop_requested()) {
                pool->m_stop_sources[thread_index] = task->stop_source;
                lock.unlock();
                pool->run_task(task);
                lock.lock();
            }
            else {
                pool->m_tasks_canceled.fetch_add(1U, std::memory_order_relaxed);
            }

            if (pool->m_work_queue.empty() && pool->m_active_threads == 0) {
                pool->m_drained_cv.notify_all();
            }
        }
    }
}

void thread_pool_private::run_task(const std::shared_ptr<work_item>& task)
{
    auto n = this->m_active_threads.fetch_add(1U, std::memory_order_relaxed) + 1U;
    atomic_fetch_max(this->m_max_active_threads, n);

    try {
        task->worker(task->stop_source.get_token());
        task->after_work(false);
        this->m_tasks_completed.fetch_add(1U, std::memory_order_relaxed);
    }
    catch (const std::exception&) {
        task->after_work(false);
        this->m_tasks_failed.fetch_add(1U, std::memory_order_relaxed);
    }

    this->m_active_threads.fetch_sub(1U, std::memory_order_relaxed);
}

}  // namespace wwa
