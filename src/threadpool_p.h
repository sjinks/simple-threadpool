#ifndef BDB9A128_2B02_4AA2_83C0_82DFF57D1267
#define BDB9A128_2B02_4AA2_83C0_82DFF57D1267

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <list>
#include <memory>
#include <mutex>
#include <stop_token>
#include <thread>
#include <vector>

#include "threadpool.h"

namespace wwa {

class thread_pool_private {
public:
    explicit thread_pool_private(std::size_t n);
    ~thread_pool_private();

    thread_pool_private(const thread_pool_private&)                = delete;
    thread_pool_private& operator=(const thread_pool_private&)     = delete;
    thread_pool_private(thread_pool_private&&) noexcept            = delete;
    thread_pool_private& operator=(thread_pool_private&&) noexcept = delete;

    thread_pool::task_t
    submit(const thread_pool::worker_t& worker, const thread_pool::after_work_t& after_work = nullptr);

    bool cancel(const thread_pool::task_t& task);
    void wait();
    bool wait_until(const std::chrono::time_point<std::chrono::steady_clock>& abs_time);

    std::size_t num_threads() const noexcept;
    std::size_t active_threads() const noexcept;
    std::size_t max_active_threads() const noexcept;
    std::size_t work_queue_size() const;
    std::size_t tasks_queued() const noexcept;
    std::size_t tasks_completed() const noexcept;
    std::size_t tasks_failed() const noexcept;
    std::size_t tasks_canceled() const noexcept;

private:
    std::size_t m_num_threads;
    std::atomic<std::size_t> m_active_threads{0};
    std::atomic<std::size_t> m_max_active_threads{0};
    std::list<std::shared_ptr<work_item>> m_work_queue;
    mutable std::mutex m_mutex;
    std::condition_variable_any m_cv;
    std::condition_variable m_drained_cv;
    std::vector<std::stop_source> m_stop_sources;
    std::vector<std::jthread> m_threads;
    std::atomic<std::size_t> m_tasks_queued{0};
    std::atomic<std::size_t> m_tasks_completed{0};
    std::atomic<std::size_t> m_tasks_failed{0};
    std::atomic<std::size_t> m_tasks_canceled{0};

    static void worker_thread(const std::stop_token& stop_token, thread_pool_private* pool, std::size_t thread_index);

    void run_task(const std::shared_ptr<work_item>& task);
};

struct work_item {
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    thread_pool::worker_t worker;
    thread_pool::after_work_t after_work;
    mutable std::stop_source stop_source;
    // NOLINTEND(misc-non-private-member-variables-in-classes)

    void stop() const { this->stop_source.request_stop(); }
    [[nodiscard]] bool stop_requested() const { return this->stop_source.stop_requested(); }
};

}  // namespace wwa

#endif /* BDB9A128_2B02_4AA2_83C0_82DFF57D1267 */
