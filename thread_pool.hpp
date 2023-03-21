#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <queue>
#include <functional>
#include <vector>
#include <memory>

class ThreadPool {
public:
    ThreadPool(const size_t thread_count);
    ~ThreadPool();

    template<typename F, typename... Args>
    decltype(auto) enqueue(F&& fn, Args&&... args);

private:
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable cv;
    std::vector<std::thread> workers;
    bool stop;
};

ThreadPool::ThreadPool(const size_t thread_count) {
    for (size_t i = 0; i < thread_count; ++i) {
        workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> locker { this->queue_mutex };
                    this->cv.wait(locker, [this] {
                        return this->stop || !this->tasks.empty();
                    });
                    if (this->stop && this->tasks.empty()) {
                        return ;
                    }
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> locker { queue_mutex };
        stop = true;
    }
    cv.notify_all();
    for (auto& worker : workers) {
        worker.join();
    }
}

template<typename F, typename... Args>
decltype(auto) ThreadPool::enqueue(F&& fn, Args&&... args) {
    using func_type = decltype(fn(args...))();
    std::function<func_type> func { std::bind(std::forward<F>(fn), std::forward<Args>(args)...) };
    auto task { std::make_shared<std::packaged_task<func_type>>(func) };
    auto result { task->get_future() };
    {
        std::unique_lock<std::mutex> locker { queue_mutex };
        if (stop) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        tasks.emplace([task] {
            (*task)();
        });
    }
    cv.notify_one();
    return result;
}