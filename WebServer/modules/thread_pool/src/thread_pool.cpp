// modules/thread_pool/src/thread_pool.cpp
#include "thread_pool.h"

ThreadPool::ThreadPool(int thread_count) : stop(false), active_tasks(0), thread_count(thread_count) {
    init_pool();
}

void ThreadPool::init_pool() {
    LOG_INFO("Initializing thread pool with %d threads", thread_count);

    for(int i = 0; i < thread_count; ++i) {
        workers.emplace_back(
            [this, i] {
                LOG_DEBUG("Worker thread %d started", i);
                for(;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                            [this]{ return this->stop || !this->tasks.empty(); });
                        if(this->stop && this->tasks.empty()) {
                            LOG_DEBUG("Worker thread %d stopping", i);
                            return;
                        }
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    this->active_tasks++;
                    LOG_DEBUG("Worker thread %d executing task", i);
                    task();
                    this->active_tasks--;
                    this->condition.notify_all();
                    LOG_DEBUG("Worker thread %d finished task", i);
                }
            }
        );
    }
}

ThreadPool::~ThreadPool() {
    LOG_INFO("Shutting down thread pool");
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for(std::thread &worker: workers) {
        worker.join();
    }
    LOG_INFO("Thread pool shut down completed");
}

void ThreadPool::wait_all() {
    LOG_DEBUG("Waiting for all tasks to complete");
    std::unique_lock<std::mutex> lock(queue_mutex);
    condition.wait(lock, [this] { 
        return stop || (tasks.empty() && active_tasks == 0); 
    });
    LOG_DEBUG("All tasks completed");
}
