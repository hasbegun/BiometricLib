#pragma once

#include <atomic>
#include <concepts>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

namespace iris {

/// Work-stealing thread pool sized to hardware concurrency.
///
/// All parallel work in the iris library is submitted to a shared pool
/// instance — no ad-hoc thread creation. The pool uses an unbounded task
/// queue; submit() never blocks.
class ThreadPool {
public:
    /// Create a pool with `num_threads` worker threads.
    /// Defaults to hardware_concurrency().
    explicit ThreadPool(size_t num_threads = 0);

    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    /// Submit a callable, return a future for its result.
    template <typename F, typename... Args>
        requires std::invocable<F, Args...>
    auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;

    /// Execute `fn(i)` for i in [begin, end) in parallel, blocking until done.
    template <typename Fn>
        requires std::invocable<Fn, size_t>
    void parallel_for(size_t begin, size_t end, Fn&& fn);

    /// Submit `count` tasks produced by `factory(i)`, return vector of futures.
    template <typename F>
    auto batch_submit(size_t count, F&& factory)
        -> std::vector<std::future<std::invoke_result_t<F, size_t>>>;

    /// Number of worker threads.
    [[nodiscard]] size_t thread_count() const noexcept;

    /// Tasks queued but not yet started.
    [[nodiscard]] size_t pending_count() const noexcept;

    /// Tasks currently being executed.
    [[nodiscard]] size_t active_count() const noexcept;

private:
    void worker_loop();

    // Order matters: workers_ must be destroyed (joined) before other members.
    // C++ destroys members in reverse declaration order, so workers_ is last.
    std::atomic<bool> stop_{false};
    std::atomic<size_t> active_{0};
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<std::function<void()>> tasks_;
    std::vector<std::jthread> workers_;
};

/// Global pool accessor. Initialized on first call. Thread-safe.
ThreadPool& global_pool();

// ---- Template implementations ----

template <typename F, typename... Args>
    requires std::invocable<F, Args...>
auto ThreadPool::submit(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>> {
    using ReturnType = std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind_front(std::forward<F>(f), std::forward<Args>(args)...));

    auto future = task->get_future();

    {
        std::lock_guard lock(mutex_);
        tasks_.emplace_back([task = std::move(task)]() { (*task)(); });
    }
    cv_.notify_one();

    return future;
}

template <typename Fn>
    requires std::invocable<Fn, size_t>
void ThreadPool::parallel_for(size_t begin, size_t end, Fn&& fn) {
    if (begin >= end) return;

    const size_t count = end - begin;
    if (count == 1) {
        fn(begin);
        return;
    }

    std::atomic<size_t> remaining{count};
    std::mutex done_mutex;
    std::condition_variable done_cv;
    std::exception_ptr first_exception;

    for (size_t i = begin; i < end; ++i) {
        std::lock_guard lock(mutex_);
        tasks_.emplace_back([&fn, i, &remaining, &done_cv, &first_exception, &done_mutex]() {
            try {
                fn(i);
            } catch (...) {
                std::lock_guard ex_lock(done_mutex);
                if (!first_exception) {
                    first_exception = std::current_exception();
                }
            }
            if (remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                done_cv.notify_one();
            }
        });
    }
    cv_.notify_all();

    std::unique_lock lock(done_mutex);
    done_cv.wait(lock, [&] { return remaining.load(std::memory_order_acquire) == 0; });

    if (first_exception) {
        std::rethrow_exception(first_exception);
    }
}

template <typename F>
auto ThreadPool::batch_submit(size_t count, F&& factory)
    -> std::vector<std::future<std::invoke_result_t<F, size_t>>> {
    using ReturnType = std::invoke_result_t<F, size_t>;
    std::vector<std::future<ReturnType>> futures;
    futures.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        futures.push_back(submit(factory, i));
    }

    return futures;
}

}  // namespace iris
