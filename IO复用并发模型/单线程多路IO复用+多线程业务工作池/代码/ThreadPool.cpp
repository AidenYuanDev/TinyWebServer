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

void ThreadPool::wait_all() {
    unique_lock<mutex> lock(queue_mutex);
    condition.wait(lock, [this] {
        return stop || (tasks.empty() && active_tasks == 0); 
    });
}

int main() {
    auto pool = make_unique<ThreadPool>(2);
    for (int i = 1; i <= 100000; i++) {
        pool->enqueue([i]{
            cout << "id" << i << endl << "thread_id:" << this_thread::get_id() << endl << endl;
        });
    }
    /*pool->wait_all();*/
    return 0;
}
