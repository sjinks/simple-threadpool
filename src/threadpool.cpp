#include "threadpool.h"
#include "threadpool_p.h"

#include <memory>
#include <stdexcept>

namespace wwa {

thread_pool::thread_pool(std::size_t n) : m_impl(std::make_unique<thread_pool_private>(n)) {}

thread_pool::~thread_pool() = default;

thread_pool::task_t thread_pool::submit(const worker_t& worker, const after_work_t& after_work)
{
    if (worker == nullptr) {
        throw std::invalid_argument("worker cannot be null");
    }

    return this->m_impl->submit(worker, after_work);
}

bool thread_pool::cancel(const thread_pool::task_t& task)
{
    return this->m_impl->cancel(task);
}

void thread_pool::wait()
{
    this->m_impl->wait();
}

bool thread_pool::wait_until(const std::chrono::time_point<std::chrono::steady_clock>& abs_time)
{
    return this->m_impl->wait_until(abs_time);
}

std::size_t thread_pool::num_threads() const noexcept
{
    return this->m_impl->num_threads();
}

std::size_t thread_pool::active_threads() const noexcept
{
    return this->m_impl->active_threads();
}

std::size_t thread_pool::max_active_threads() const noexcept
{
    return this->m_impl->max_active_threads();
}

std::size_t thread_pool::work_queue_size() const
{
    return this->m_impl->work_queue_size();
}

std::size_t thread_pool::tasks_queued() const noexcept
{
    return this->m_impl->tasks_queued();
}

std::size_t thread_pool::tasks_completed() const noexcept
{
    return this->m_impl->tasks_completed();
}

std::size_t thread_pool::tasks_failed() const noexcept
{
    return this->m_impl->tasks_failed();
}

std::size_t thread_pool::tasks_canceled() const noexcept
{
    return this->m_impl->tasks_canceled();
}

}  // namespace wwa
