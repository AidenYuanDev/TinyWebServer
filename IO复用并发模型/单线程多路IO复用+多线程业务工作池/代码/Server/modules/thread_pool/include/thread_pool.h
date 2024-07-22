#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
using namespace std;

class ThreadPool {
public:
    explicit ThreadPool(const int num_thread);
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ~ThreadPool();

    template <typename F, typename... Args>
    auto enqueue(F &&f, Args &&...args) -> future<decltype(f(args...))>;
    void wait_all(); 

private:
    vector<thread> workers;
    queue<function<void()>> tasks;

    mutex queue_mutex;
    condition_variable condition;
    atomic<bool> stop;
    atomic<int> active_tasks;  
};

template <class F, class... Args>
auto ThreadPool::enqueue(F &&f, Args &&...args) -> future<decltype(f(args...))> {
    using return_type = decltype(f(args...));
    auto task = make_shared<packaged_task<return_type()>>(bind(std::forward<F>(f), std::forward<Args>(args)...));

    future<return_type> res = task->get_future();
    {
        unique_lock<mutex> lock(queue_mutex);
        if (stop) {
            runtime_error("enqueue on stopped ThreadPool");
        }
        tasks.emplace([task] { (*task)(); });
    }
    condition.notify_one();
    return res;
}

