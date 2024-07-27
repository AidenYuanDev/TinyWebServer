#include <cstdint>
#include <gtest/gtest.h>
#include "thread_pool.h"
#include <atomic>
#include <chrono>
#include <numeric>

class ThreadPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        pool = new ThreadPool(4);
    }

    void TearDown() override {
        delete pool;
    }

    ThreadPool* pool;
};

TEST_F(ThreadPoolTest, TaskExecution) {
    std::atomic<int> counter(0);

    cout << "star:" << endl;
    for (int i = 0; i < 100; ++i) {
        pool->enqueue([&counter]() {
            counter++;
        });
    }

    pool->wait_all();
    EXPECT_EQ(counter, 100);
}

TEST_F(ThreadPoolTest, ExceptionHandling) {
    EXPECT_NO_THROW({
        pool->enqueue([]() {
            throw std::runtime_error("Test exception");
        });
        pool->wait_all();
    });
}

// 测试任务执行时间
TEST_F(ThreadPoolTest, TaskExecutionTime) {
    const int num_tasks = 1000;
    std::vector<std::future<void>> results;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_tasks; ++i) {
        results.emplace_back(pool->enqueue([]() {
            // 模拟一些工作
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }));
    }

    // 等待所有任务完成
    for (auto& result : results) {
        result.get();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Execution time for " << num_tasks << " tasks: " << duration.count() << "ms" << std::endl;

    // 检查执行时间是否在合理范围内
    EXPECT_LT(duration.count(), num_tasks); // 应该比顺序执行快得多
}

// 测试线程池的并发性能
TEST_F(ThreadPoolTest, ConcurrencyPerformance) {
    const int num_tasks = 100;
    std::vector<std::future<int64_t>> results;

    for (int i = 0; i < num_tasks; ++i) {
        results.emplace_back(pool->enqueue([i]() {
            // 模拟一些计算密集型工作
            int64_t sum = 0;
            for (int j = 0; j < 1000000; ++j) {
                sum += j;
            }
            return sum;
        }));
    }

    // 收集结果
    std::vector<int64_t> sums;
    for (auto& result : results) {
        sums.push_back(result.get());
    }

    // 检查结果
    EXPECT_EQ(sums.size(), num_tasks);
    int64_t expected_sum = static_cast<int64_t>(num_tasks) * 499999500000; // 使用 int64_t 避免溢出
    EXPECT_EQ(std::accumulate(sums.begin(), sums.end(), static_cast<int64_t>(0)), expected_sum);
}

// 测试线程池在高负载下的性能
TEST_F(ThreadPoolTest, HighLoadPerformance) {
    const int num_tasks = 10000;
    std::vector<std::future<int>> results;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_tasks; ++i) {
        results.emplace_back(pool->enqueue([i]() {
            // 模拟短暂但频繁的任务
            return i * i;
        }));
    }

    // 收集结果
    std::vector<int> squares;
    for (auto& result : results) {
        squares.push_back(result.get());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "High load execution time for " << num_tasks << " tasks: " << duration.count() << "ms" << std::endl;

    // 检查结果
    EXPECT_EQ(squares.size(), num_tasks);
    for (int i = 0; i < num_tasks; ++i) {
        EXPECT_EQ(squares[i], i * i);
    }
}
