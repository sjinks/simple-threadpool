#ifndef E2702E5A_CA58_4C98_814D_7AF7376E069A
#define E2702E5A_CA58_4C98_814D_7AF7376E069A

#include <future>
#include <optional>
#include <utility>

#include "threadpool.h"

namespace wwa {

template<typename R>
using task_after_work_void_t = void (*)(const std::optional<R>&);

template<typename R, typename T>
using task_after_work_t = void (*)(const std::optional<R>&, const T&);

template<typename R, typename... Args>
inline std::shared_future<R> submit_packaged_task(
    thread_pool& pool, task_after_work_void_t<R> after, std::packaged_task<R(const std::stop_token&, Args...)>&& task,
    Args&&... args
)
{
    auto future   = task.get_future().share();
    auto task_ptr = std::make_shared<std::packaged_task<R(const std::stop_token&, Args...)>>(std::move(task));
    pool.submit(
        [task_ptr, ... a = std::forward<Args>(args)](const std::stop_token& token) mutable {
            std::apply(std::move(*task_ptr), std::make_tuple(token, std::forward<decltype(a)>(a)...));
        },
        [after, future](bool canceled) {
            if (after) {
                after(canceled ? std::nullopt : std::make_optional(future.get()));
            }
        }
    );

    return future;
}

template<typename R, typename... Args, typename T>
inline std::shared_future<R> submit_packaged_task(
    thread_pool& pool, task_after_work_t<R, T> after, const T& extra,
    std::packaged_task<R(const std::stop_token&, Args...)>&& task, Args&&... args
)
{
    auto future   = task.get_future().share();
    auto task_ptr = std::make_shared<std::packaged_task<R(const std::stop_token&, Args...)>>(std::move(task));
    pool.submit(
        [task_ptr, ... a = std::forward<Args>(args)](const std::stop_token& token) mutable {
            std::apply(std::move(*task_ptr), std::make_tuple(token, std::forward<decltype(a)>(a)...));
        },
        [after, future, &extra](bool canceled) {
            if (after) {
                after(canceled ? std::nullopt : std::make_optional(future.get()), extra);
            }
        }
    );

    return future;
}

template<typename R, typename... Args>
inline std::shared_future<R>
submit_packaged_task(thread_pool& pool, std::packaged_task<R(const std::stop_token&, Args...)>&& task, Args&&... args)
{
    return submit_packaged_task(
        pool, static_cast<task_after_work_void_t<R>>(nullptr), std::move(task), std::forward<Args>(args)...
    );
}

template<typename R, typename... Args>
std::shared_future<R> submit_packaged_task(
    thread_pool& pool, task_after_work_void_t<R> after, std::packaged_task<R(Args...)>&& task, Args&&... args
)
{
    auto future   = task.get_future().share();
    auto task_ptr = std::make_shared<std::packaged_task<R(Args...)>>(std::move(task));
    pool.submit(
        [task_ptr, ... a = std::forward<Args>(args)](const std::stop_token&) mutable {
            std::apply(std::move(*task_ptr), std::make_tuple(std::forward<decltype(a)>(a)...));
        },
        [after, future](bool canceled) {
            if (after) {
                after(canceled ? std::nullopt : std::make_optional(future.get()));
            }
        }
    );

    return future;
}

template<typename R, typename... Args, typename T>
std::shared_future<R> submit_packaged_task(
    thread_pool& pool, task_after_work_t<R, T> after, const T& extra, std::packaged_task<R(Args...)>&& task,
    Args&&... args
)
{
    auto future   = task.get_future().share();
    auto task_ptr = std::make_shared<std::packaged_task<R(Args...)>>(std::move(task));
    pool.submit(
        [task_ptr, ... a = std::forward<Args>(args)](const std::stop_token&) mutable {
            std::apply(std::move(*task_ptr), std::make_tuple(std::forward<decltype(a)>(a)...));
        },
        [after, future, &extra](bool canceled) {
            if (after) {
                after(canceled ? std::nullopt : std::make_optional(future.get()), extra);
            }
        }
    );

    return future;
}

template<typename R, typename... Args>
inline std::shared_future<R>
submit_packaged_task(thread_pool& pool, std::packaged_task<R(Args...)>&& task, Args&&... args)
{
    return submit_packaged_task(
        pool, static_cast<task_after_work_void_t<R>>(nullptr), std::move(task), std::forward<Args>(args)...
    );
}

}  // namespace wwa

#endif /* E2702E5A_CA58_4C98_814D_7AF7376E069A */
