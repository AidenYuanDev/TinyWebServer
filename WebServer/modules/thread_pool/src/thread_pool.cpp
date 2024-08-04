// thread_pool.cpp
#include "thread_pool.h"

ThreadPool::ThreadPool() : stop(false), active_tasks(0) {
    init_pool();
}

void ThreadPool::init_pool() {
    auto& config = ConfigManager::getInstance();
    int thread_count = config.getThreadPoolSize();

    for(int i = 0; i < thread_count; ++i) {
        workers.emplace_back(
            [this] {
                for(;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                            [this]{ return this->stop || !this->tasks.empty(); });
                        if(this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    this->active_tasks++;
                    task();
                    this->active_tasks--;
                    this->condition.notify_all();
                }
            }
        );
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for(std::thread &worker: workers)
        worker.join();
}

void ThreadPool::wait_all() {
    std::unique_lock<std::mutex> lock(queue_mutex);
    condition.wait(lock, [this] { 
        return stop || (tasks.empty() && active_tasks == 0); 
    });
}
