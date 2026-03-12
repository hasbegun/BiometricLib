#include <eyetrack/core/thread_pool.hpp>

namespace eyetrack {

ThreadPool::ThreadPool(size_t num_threads) {
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 4;  // fallback
    }

    workers_.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this] { worker_loop(); });
    }
}

ThreadPool::~ThreadPool() {
    stop_.store(true, std::memory_order_release);
    cv_.notify_all();
    // jthread destructor requests stop and joins automatically
}

void ThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, [this] {
                return stop_.load(std::memory_order_acquire) || !tasks_.empty();
            });

            if (stop_.load(std::memory_order_acquire) && tasks_.empty()) {
                return;
            }

            task = std::move(tasks_.front());
            tasks_.pop_front();
        }

        active_.fetch_add(1, std::memory_order_relaxed);
        task();
        active_.fetch_sub(1, std::memory_order_relaxed);
    }
}

size_t ThreadPool::thread_count() const noexcept {
    return workers_.size();
}

size_t ThreadPool::pending_count() const noexcept {
    std::lock_guard lock(mutex_);
    return tasks_.size();
}

size_t ThreadPool::active_count() const noexcept {
    return active_.load(std::memory_order_relaxed);
}

ThreadPool& global_pool() {
    static ThreadPool pool;
    return pool;
}

}  // namespace eyetrack
