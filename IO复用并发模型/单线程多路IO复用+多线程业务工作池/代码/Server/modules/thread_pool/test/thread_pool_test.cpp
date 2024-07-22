#include <gtest/gtest.h>
#include "thread_pool.h"
#include <atomic>
#include <chrono>

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

TEST_F(ThreadPoolTest, ConcurrencyPerformance) {
    const int NUM_TASKS = 1000000;
    std::atomic<int> counter(0);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_TASKS; ++i) {
        pool->enqueue([&counter]() {
            counter++;
        });
    }

    pool->wait_all();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    EXPECT_NEAR(counter, NUM_TASKS, 1e-5);
    std::cout << "Time taken: " << duration.count() << " ms" << std::endl;
}
