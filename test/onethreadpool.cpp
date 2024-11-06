#include <chrono>
#include <mutex>
#include <numeric>
#include <vector>
#include <gtest/gtest.h>
#include "threadpool.h"
#include "threadpool_p.h"  // IWYU pragma: keep: complete definition of work_item

using unique_lock = std::unique_lock<std::mutex>;

const auto empty_task = [](const std::stop_token &) { /* Do nothing */ };

class OneThreadPoolTest : public ::testing::Test {
protected:
    void SetUp() override { this->m_pool = std::make_unique<wwa::thread_pool>(1); }

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    std::unique_ptr<wwa::thread_pool> m_pool;
    std::vector<int> m_results;
    std::mutex m_mutex;
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

TEST_F(OneThreadPoolTest, SequentialExecution)
{
    constexpr auto NUM_TASKS = 10;
    std::vector<int> expected(NUM_TASKS);
    std::iota(expected.begin(), expected.end(), 0);

    {
        const unique_lock lock(this->m_mutex);

        for (int i = 0; i < NUM_TASKS; ++i) {
            this->m_pool->submit([this, i](const std::stop_token &) {
                const unique_lock lck(this->m_mutex);
                this->m_results.push_back(i);
            });
        }

        EXPECT_EQ(this->m_pool->num_threads(), 1);
        EXPECT_LE(this->m_pool->active_threads(), 1);
        EXPECT_LE(this->m_pool->max_active_threads(), 1);
        EXPECT_LE(this->m_pool->work_queue_size(), NUM_TASKS);
        EXPECT_GE(this->m_pool->work_queue_size(), NUM_TASKS - 1);
    }

    this->m_pool->wait();

    EXPECT_EQ(this->m_results, expected);
    EXPECT_EQ(this->m_pool->max_active_threads(), 1);
}

TEST_F(OneThreadPoolTest, Cancel)
{
    constexpr auto NUM_TASKS = 2;

    std::vector<int> expected(NUM_TASKS);
    std::vector<wwa::thread_pool::task_t> tasks;

    std::iota(expected.begin(), expected.end(), 0);
    tasks.reserve(NUM_TASKS);

    {
        const unique_lock lock(this->m_mutex);

        for (int i = 0; i < NUM_TASKS; ++i) {
            tasks.push_back(this->m_pool->submit([this, i](const std::stop_token &) {
                const unique_lock lck(this->m_mutex);
                this->m_results.push_back(i);
            }));
        }

        ASSERT_EQ(tasks.size(), NUM_TASKS);
        auto result = this->m_pool->cancel(tasks[NUM_TASKS - 1]);
        EXPECT_TRUE(result);

        expected.pop_back();
    }

    this->m_pool->wait();

    EXPECT_EQ(this->m_results, expected);
    EXPECT_EQ(this->m_pool->tasks_canceled(), 1);
}

TEST_F(OneThreadPoolTest, CancelCompletedTask)
{
    auto task = this->m_pool->submit(empty_task);
    this->m_pool->wait();
    auto result = this->m_pool->cancel(task);
    EXPECT_FALSE(result);
    EXPECT_EQ(this->m_pool->tasks_canceled(), 0);
}

TEST_F(OneThreadPoolTest, DoubleCancel)
{
    {
        const unique_lock lock(this->m_mutex);

        this->m_pool->submit([this](const std::stop_token &) { const unique_lock lck(this->m_mutex); });
        auto task = this->m_pool->submit(empty_task);

        auto sp_task = task.lock();
        ASSERT_TRUE(sp_task);

        bool result = this->m_pool->cancel(task);
        EXPECT_TRUE(result);
        result = this->m_pool->cancel(task);
        EXPECT_FALSE(result);
    }

    this->m_pool->wait();
    EXPECT_EQ(this->m_pool->tasks_canceled(), 1);
}

TEST_F(OneThreadPoolTest, CancelQueuedTask)
{
    {
        const unique_lock lock(this->m_mutex);

        this->m_pool->submit([this](const std::stop_token &) { const unique_lock lck(this->m_mutex); });
        auto task = this->m_pool->submit(empty_task);

        auto sp_task = task.lock();
        ASSERT_TRUE(sp_task);

        sp_task->stop();
        EXPECT_TRUE(sp_task->stop_requested());
    }

    this->m_pool->wait();
    EXPECT_EQ(this->m_pool->tasks_canceled(), 1);
}

TEST_F(OneThreadPoolTest, Wait)
{
    {
        const unique_lock lock(this->m_mutex);
        this->m_pool->submit([this](const std::stop_token &) { const unique_lock lck(this->m_mutex); });
        EXPECT_FALSE(this->m_pool->wait_for(std::chrono::microseconds(10)));
    }

    EXPECT_TRUE(this->m_pool->wait_until(std::chrono::system_clock::now() + std::chrono::milliseconds(1)));
}

TEST_F(OneThreadPoolTest, WaitUntilPast)
{
    EXPECT_FALSE(this->m_pool->wait_until(std::chrono::system_clock::now() - std::chrono::seconds(1)));
}
