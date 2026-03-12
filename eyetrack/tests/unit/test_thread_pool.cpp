#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <numeric>
#include <vector>

#include <eyetrack/core/thread_pool.hpp>

namespace {

using namespace eyetrack;

TEST(ThreadPool, submit_and_get_result) {
    ThreadPool pool(2);
    auto future = pool.submit([]() { return 42; });
    EXPECT_EQ(future.get(), 42);
}

TEST(ThreadPool, parallel_for_processes_all_elements) {
    ThreadPool pool(4);
    constexpr size_t N = 100;
    std::vector<std::atomic<int>> vec(N);

    pool.parallel_for(0, N, [&vec](size_t i) {
        vec[i].store(static_cast<int>(i * 2), std::memory_order_relaxed);
    });

    for (size_t i = 0; i < N; ++i) {
        EXPECT_EQ(vec[i].load(std::memory_order_relaxed), static_cast<int>(i * 2))
            << "Element " << i << " was not processed correctly";
    }
}

TEST(ThreadPool, multiple_concurrent_tasks) {
    ThreadPool pool(4);
    constexpr size_t N = 100;
    std::vector<std::future<int>> futures;
    futures.reserve(N);

    for (size_t i = 0; i < N; ++i) {
        futures.push_back(pool.submit([i]() { return static_cast<int>(i); }));
    }

    for (size_t i = 0; i < N; ++i) {
        EXPECT_EQ(futures[i].get(), static_cast<int>(i));
    }
}

TEST(ThreadPool, thread_count_matches_requested) {
    ThreadPool pool(3);
    EXPECT_EQ(pool.thread_count(), 3U);
}

TEST(ThreadPool, destructor_joins_threads) {
    // If destructor doesn't join properly, ASan will detect leaks
    {
        ThreadPool pool(2);
        pool.submit([]() { return 0; }).get();
    }
    // If we reach here without hanging or crashing, the test passes
    SUCCEED();
}

}  // namespace
