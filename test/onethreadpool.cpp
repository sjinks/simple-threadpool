#include <mutex>
#include <numeric>
#include <vector>
#include <gtest/gtest.h>
#include "threadpool.h"
#include "threadpool_p.h"  // IWYU pragma: keep: complete definition of work_item

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

    std::unique_lock<std::mutex> lock(this->m_mutex);

    for (int i = 0; i < NUM_TASKS; ++i) {
        m_pool->submit([this, i](const std::stop_token &) {
            const std::unique_lock<std::mutex> lck(this->m_mutex);
            this->m_results.push_back(i);
        });
    }

    EXPECT_EQ(this->m_pool->num_threads(), 1);
    EXPECT_LE(this->m_pool->active_threads(), 1);
    EXPECT_LE(this->m_pool->max_active_threads(), 1);
    EXPECT_LE(this->m_pool->work_queue_size(), NUM_TASKS);
    EXPECT_GE(this->m_pool->work_queue_size(), NUM_TASKS - 1);

    lock.unlock();
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

    std::unique_lock<std::mutex> lock(this->m_mutex);

    for (int i = 0; i < NUM_TASKS; ++i) {
        tasks.push_back(this->m_pool->submit([this, i](const std::stop_token &) {
            const std::unique_lock<std::mutex> lck(this->m_mutex);
            this->m_results.push_back(i);
        }));
    }

    ASSERT_EQ(tasks.size(), NUM_TASKS);
    auto result = this->m_pool->cancel(tasks[NUM_TASKS - 1]);
    EXPECT_TRUE(result);

    expected.pop_back();

    lock.unlock();
    this->m_pool->wait();

    EXPECT_EQ(this->m_results, expected);
    EXPECT_EQ(this->m_pool->tasks_canceled(), 1);
}

TEST_F(OneThreadPoolTest, CancelCompletedTask)
{
    auto task = this->m_pool->submit([](const std::stop_token &) { /* Do nothing */ });
    this->m_pool->wait();
    auto result = this->m_pool->cancel(task);
    EXPECT_FALSE(result);
    EXPECT_EQ(this->m_pool->tasks_canceled(), 0);
}

TEST_F(OneThreadPoolTest, DoubleCancel)
{
    std::unique_lock<std::mutex> lock(this->m_mutex);

    this->m_pool->submit([this](const std::stop_token &) { const std::unique_lock<std::mutex> lck(this->m_mutex); });
    auto task = this->m_pool->submit([](const std::stop_token &) { /* Do nothing */ });

    auto sp_task = task.lock();
    ASSERT_TRUE(sp_task);

    bool result = this->m_pool->cancel(task);
    EXPECT_TRUE(result);
    result = this->m_pool->cancel(task);
    EXPECT_FALSE(result);

    lock.unlock();
    this->m_pool->wait();
    EXPECT_EQ(this->m_pool->tasks_canceled(), 1);
}

TEST_F(OneThreadPoolTest, CancelQueuedTask)
{
    std::unique_lock<std::mutex> lock(this->m_mutex);

    this->m_pool->submit([this](const std::stop_token &) { const std::unique_lock<std::mutex> lck(this->m_mutex); });
    auto task = this->m_pool->submit([](const std::stop_token &) { /* Do nothing */ });

    auto sp_task = task.lock();
    ASSERT_TRUE(sp_task);

    sp_task->stop();
    EXPECT_TRUE(sp_task->stop_requested());

    lock.unlock();
    this->m_pool->wait();
    EXPECT_EQ(this->m_pool->tasks_canceled(), 1);
}
