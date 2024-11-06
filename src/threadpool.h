#ifndef DB57AD06_3B44_40FF_A554_841402EFC389
#define DB57AD06_3B44_40FF_A554_841402EFC389

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <stop_token>

#include "export.h"

namespace wwa {

struct work_item;

class thread_pool_private;
class WWA_SIMPLE_THREADPOOL_EXPORT thread_pool {
public:
    using task_t       = std::weak_ptr<work_item>;
    using worker_t     = std::function<void(const std::stop_token&)>;
    using after_work_t = std::function<void(bool)>;

    explicit thread_pool(std::size_t n = 0);
    ~thread_pool();

    thread_pool(const thread_pool&)                = delete;
    thread_pool& operator=(const thread_pool&)     = delete;
    thread_pool(thread_pool&&) noexcept            = default;
    thread_pool& operator=(thread_pool&&) noexcept = default;

    task_t submit(const worker_t& worker, const after_work_t& after_work = nullptr);
    bool cancel(const task_t& task);
    void wait();

    template<typename Rep, typename Period>
    bool wait_for(const std::chrono::duration<Rep, Period>& rel_time)
    {
        return this->wait_until(std::chrono::steady_clock::now() + rel_time);
    }

    template<typename Clock, typename Duration>
    bool wait_until(const std::chrono::time_point<Clock, Duration>& abs_time)
    {
        auto now = Clock::now();
        if (abs_time <= now) {
            return false;
        }

        auto duration_until_abs_time = abs_time - now;
        auto steady_abs_time         = std::chrono::steady_clock::now() + duration_until_abs_time;

        return this->wait_until(steady_abs_time);
    }

    bool wait_until(const std::chrono::time_point<std::chrono::steady_clock>& abs_time);

    [[nodiscard]] std::size_t num_threads() const noexcept;
    [[nodiscard]] std::size_t active_threads() const noexcept;
    [[nodiscard]] std::size_t max_active_threads() const noexcept;
    [[nodiscard]] std::size_t work_queue_size() const;
    [[nodiscard]] std::size_t tasks_queued() const noexcept;
    [[nodiscard]] std::size_t tasks_completed() const noexcept;
    [[nodiscard]] std::size_t tasks_failed() const noexcept;
    [[nodiscard]] std::size_t tasks_canceled() const noexcept;

private:
    std::unique_ptr<thread_pool_private> m_impl;
};

}  // namespace wwa

#endif /* DB57AD06_3B44_40FF_A554_841402EFC389 */
