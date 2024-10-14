#ifndef DB57AD06_3B44_40FF_A554_841402EFC389
#define DB57AD06_3B44_40FF_A554_841402EFC389

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
