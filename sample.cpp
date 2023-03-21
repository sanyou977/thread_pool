#include <iostream>
#include <chrono>
#include <vector>
#include <future>

#include "thread_pool.hpp"

int func(int a, int b) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << a << ' ' << b << std::endl;
    return a + b;
}

int main() {
    ThreadPool pool(4);

    std::vector<std::future<int>> result;
    for (int i = 0; i < 8; ++i) {
        result.emplace_back(
            pool.enqueue(func, i, i + 1)
        );
    }

    for (auto& res : result) {
        res.wait();
    }

    for (auto& res : result) {
        std::cout << "res: " << res.get() << std::endl;
    }

    return 0;
}