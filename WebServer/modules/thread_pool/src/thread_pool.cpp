#include "thread_pool.h"

ThreadPool::ThreadPool(const int num_thread) : stop(false) {
    workers.reserve(num_thread);
    for (int i = 1; i <= num_thread; i++) {
        workers.emplace_back([this] {
            while (true) {
                function<void()> task;
                {
                    unique_lock<mutex> lock(queue_mutex);
                    condition.wait(lock, [this] { return stop || !tasks.empty(); });
                    if (stop && tasks.empty()) return;
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                active_tasks++;
                task();
                active_tasks--;
                condition.notify_all();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        unique_lock<mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (auto &worker : workers) worker.join();
}

void ThreadPool::wait_all() {
    unique_lock<mutex> lock(queue_mutex);
    condition.wait(lock, [this] {
        return stop || (tasks.empty() && active_tasks == 0); 
    });
}
