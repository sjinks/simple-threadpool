#include <gtest/gtest.h>

#include "../src/packaged_task.h"
#include "../src/threadpool.h"

class PackagedTaskTest : public ::testing::Test {
protected:
    void SetUp() override { this->m_pool = std::make_unique<wwa::thread_pool>(1); }

    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    std::unique_ptr<wwa::thread_pool> m_pool;
};

TEST_F(PackagedTaskTest, PackagedTask_AE)
{
    bool task_invoked = false;

    std::packaged_task<double(const double&)> task([&task_invoked](const double& x) {
        task_invoked = true;
        return x * x;
    });

    wwa::task_after_work_t<double, double> after = [](const std::optional<double>& result, const double& extra) {
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), extra * extra);
    };

    constexpr auto param = 10.0;
    wwa::submit_packaged_task(*this->m_pool, after, param, std::move(task), param);
    this->m_pool->wait();
    EXPECT_TRUE(task_invoked);
}

TEST_F(PackagedTaskTest, PackagedTask_A)
{
    bool task_invoked    = false;
    constexpr auto param = 10.0;

    std::packaged_task<double(const double&)> task([&task_invoked](const double& x) {
        task_invoked = true;
        return x * x;
    });

    wwa::task_after_work_void_t<double> after = [](const std::optional<double>& result) {
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), param * param);
    };

    wwa::submit_packaged_task(*this->m_pool, after, std::move(task), param);
    this->m_pool->wait();
    EXPECT_TRUE(task_invoked);
}

TEST_F(PackagedTaskTest, PackagedTask)
{
    bool task_invoked    = false;
    constexpr auto param = 10.0;

    std::packaged_task<double(const double&)> task([&task_invoked](const double& x) {
        task_invoked = true;
        return x * x;
    });

    wwa::submit_packaged_task(*this->m_pool, std::move(task), param);
    this->m_pool->wait();
    EXPECT_TRUE(task_invoked);
}

TEST_F(PackagedTaskTest, PackagedTask_SAE)
{
    bool task_invoked = false;

    std::packaged_task<double(const std::stop_token&, const double&)> task(
        [&task_invoked](const std::stop_token&, const double& x) {
            task_invoked = true;
            return x * x;
        }
    );

    wwa::task_after_work_t<double, double> after = [](const std::optional<double>& result, const double& extra) {
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), extra * extra);
    };

    constexpr auto param = 10.0;
    wwa::submit_packaged_task(*this->m_pool, after, param, std::move(task), param);
    this->m_pool->wait();
    EXPECT_TRUE(task_invoked);
}

TEST_F(PackagedTaskTest, PackagedTask_SA)
{
    bool task_invoked    = false;
    constexpr auto param = 10.0;

    std::packaged_task<double(const std::stop_token&, const double&)> task(
        [&task_invoked](const std::stop_token&, const double& x) {
            task_invoked = true;
            return x * x;
        }
    );

    wwa::task_after_work_void_t<double> after = [](const std::optional<double>& result) {
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), param * param);
    };

    wwa::submit_packaged_task(*this->m_pool, after, std::move(task), param);
    this->m_pool->wait();
    EXPECT_TRUE(task_invoked);
}

TEST_F(PackagedTaskTest, PackagedTask_S)
{
    bool task_invoked    = false;
    constexpr auto param = 10.0;

    std::packaged_task<double(const std::stop_token&, const double&)> task(
        [&task_invoked](const std::stop_token&, const double& x) {
            task_invoked = true;
            return x * x;
        }
    );

    wwa::submit_packaged_task(*this->m_pool, std::move(task), param);
    this->m_pool->wait();
    EXPECT_TRUE(task_invoked);
}
