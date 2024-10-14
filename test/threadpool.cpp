#include "threadpool.h"
#include <condition_variable>
#include <exception>
#include <functional>
#include <latch>
#include <numeric>
#include <semaphore>
#include <thread>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class ThreadPoolTest : public ::testing::Test {
protected:
    void SetUp() override { this->m_pool = std::make_unique<wwa::thread_pool>(ThreadPoolTest::NUM_THREADS); }

    static constexpr auto NUM_THREADS = 4U;
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    std::unique_ptr<wwa::thread_pool> m_pool;
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

TEST_F(ThreadPoolTest, InitialValues)
{
    EXPECT_EQ(this->m_pool->num_threads(), ThreadPoolTest::NUM_THREADS);
    EXPECT_EQ(this->m_pool->active_threads(), 0);
    EXPECT_EQ(this->m_pool->max_active_threads(), 0);
    EXPECT_EQ(this->m_pool->work_queue_size(), 0);
    EXPECT_EQ(this->m_pool->tasks_queued(), 0);
    EXPECT_EQ(this->m_pool->tasks_completed(), 0);
    EXPECT_EQ(this->m_pool->tasks_failed(), 0);
    EXPECT_EQ(this->m_pool->tasks_canceled(), 0);
}

TEST_F(ThreadPoolTest, WaitOnEmptyQueue)
{
    EXPECT_EQ(this->m_pool->work_queue_size(), 0);
    this->m_pool->wait();
    EXPECT_EQ(this->m_pool->work_queue_size(), 0);
}

TEST_F(ThreadPoolTest, Counters)
{
    int first_task_has_exception  = -1;
    int second_task_has_exception = -1;

    this->m_pool->submit(
        [](const std::stop_token &) { throw std::runtime_error("Task failed"); },
        [&first_task_has_exception](bool) { first_task_has_exception = (std::current_exception() != nullptr) ? 1 : 0; }
    );

    this->m_pool->submit(
        [](const std::stop_token &) { /* Do nothing */ },
        [&second_task_has_exception](bool) {
            second_task_has_exception = (std::current_exception() != nullptr) ? 1 : 0;
        }
    );

    m_pool->wait();

    EXPECT_EQ(m_pool->tasks_queued(), 2);
    EXPECT_EQ(m_pool->tasks_completed(), 1);
    EXPECT_EQ(m_pool->tasks_failed(), 1);
    EXPECT_EQ(this->m_pool->work_queue_size(), 0);

    EXPECT_EQ(first_task_has_exception, 1);
    EXPECT_EQ(second_task_has_exception, 0);
}

TEST_F(ThreadPoolTest, ParallelExecution)
{
    std::counting_semaphore<ThreadPoolTest::NUM_THREADS> sem{0};
    std::latch latch(ThreadPoolTest::NUM_THREADS);

    std::vector<unsigned int> expected(ThreadPoolTest::NUM_THREADS);
    std::iota(expected.begin(), expected.end(), 0);

    std::vector<unsigned int> results;
    results.reserve(ThreadPoolTest::NUM_THREADS);

    for (auto i = 0U; i < ThreadPoolTest::NUM_THREADS; ++i) {
        m_pool->submit([&results, &sem, &latch, i](const std::stop_token &) {
            latch.count_down();
            sem.acquire();
            results.push_back(i);
            sem.release();
        });
    }

    while (!latch.try_wait()) {
        std::this_thread::yield();
        constexpr auto DELAY = 10U;
        std::this_thread::sleep_for(std::chrono::microseconds(DELAY));
    }

    EXPECT_EQ(this->m_pool->max_active_threads(), ThreadPoolTest::NUM_THREADS);
    EXPECT_EQ(this->m_pool->active_threads(), ThreadPoolTest::NUM_THREADS);
    EXPECT_EQ(this->m_pool->work_queue_size(), 0);
    EXPECT_EQ(this->m_pool->tasks_queued(), ThreadPoolTest::NUM_THREADS);
    EXPECT_EQ(this->m_pool->tasks_completed(), 0);
    EXPECT_EQ(this->m_pool->tasks_failed(), 0);

    sem.release();
    this->m_pool->wait();

    EXPECT_EQ(this->m_pool->tasks_completed(), ThreadPoolTest::NUM_THREADS);
    EXPECT_THAT(results, ::testing::UnorderedElementsAreArray(expected.begin(), expected.end()));
}

TEST(ConstructionDestructionTest, DefaultConstruction)
{
    const wwa::thread_pool pool;
    EXPECT_EQ(pool.num_threads(), std::thread::hardware_concurrency());
    EXPECT_GE(pool.num_threads(), 1);
}

TEST(ConstructionDestructionTest, DestructionWithActiveTasks)
{
    constexpr auto NUM_THREADS = 4U;
    constexpr auto NUM_TASKS   = NUM_THREADS * 2U;
    std::latch latch(NUM_THREADS);

    {
        wwa::thread_pool pool(NUM_THREADS);
        for (auto i = 0U; i < NUM_TASKS; ++i) {
            pool.submit(
                [&latch](const std::stop_token &token) {
                    latch.count_down();
                    std::mutex m;
                    std::unique_lock<std::mutex> lock(m);
                    std::condition_variable_any().wait(lock, token, [] { return false; });
                },
                ((i % 2) != 0U) ? [](bool) { /* Do nothing */ } : std::function<void(bool)>()
            );
        }

        while (!latch.try_wait()) {
            std::this_thread::yield();
            constexpr auto DELAY = 10U;
            std::this_thread::sleep_for(std::chrono::microseconds(DELAY));
        }

        EXPECT_EQ(pool.active_threads(), NUM_THREADS);
        EXPECT_EQ(pool.work_queue_size(), NUM_TASKS - NUM_THREADS);
        EXPECT_EQ(pool.tasks_queued(), NUM_TASKS);
    }
}

TEST(ArgumentValidityTest, SubmitNullWorker)
{
    wwa::thread_pool pool;
    EXPECT_THROW(pool.submit(nullptr), std::invalid_argument);
}

// CXXFLAGS: "-O1 -g -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls"
// TEST(ASAN, ASANTest)
// {
//     volatile int index  = 10;
//     volatile int* array = new int[100];
//     delete[] array;
//     array[index] = 2 * index;
// }

// #include <cstdlib>

// TEST(ASAN, LSANTest)
// {
//     volatile int* p = static_cast<volatile int*>(std::malloc(7 * sizeof(int)));
//     *p              = 0;
//     p               = nullptr;
// }

// #include <pthread.h>
// namespace {

// int Global;
// void *Thread1(void *x)
// {
//     Global = 42;
//     return x;
// }

// int check()
// {
//     pthread_t t;
//     pthread_create(&t, NULL, Thread1, NULL);
//     Global = 43;
//     pthread_join(t, NULL);
//     return Global;
// }

// }  // namespace

// // g++: needs sysctl vm.mmap_rnd_bits=30, see https://stackoverflow.com/a/77856955
// // CXXFLAGS="-O1 -g -fsanitize=thread"
// TEST(TSAN, TSANTest)
// {
//     EXPECT_EQ(check(), 43);
// }

// clang++ only
// CXXFLAGS="-O1 -g -fsanitize=memory -fno-omit-frame-pointer -fno-optimize-sibling-calls
// -fsanitize-memory-track-origins -fsanitize-recover=all" too many FPs because all libraries need to be instrumented
// TEST(MSAN, MSANTest)
// {
//     volatile int *p = new int;
//     *p              = 0;
//     delete p;
// }

// CXXFLAGS="-O1 -g -fsanitize=undefined -fsanitize=nullability -fsanitize=integer -fsanitize=implicit-conversion
// -fsanitize=float-divide-by-zero -fsanitize=local-bounds -fno-omit-frame-pointer"
TEST(UBSAN, UBSANTest)
{
    int k  = 0x7fff'ffff;
    k     += 1;
    EXPECT_EQ(k, 0x8000'0000);
}
