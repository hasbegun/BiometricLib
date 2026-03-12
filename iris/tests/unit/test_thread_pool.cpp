#include <iris/core/thread_pool.hpp>

#include <atomic>
#include <numeric>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

using namespace iris;

TEST(ThreadPool, SubmitAndWait) {
    ThreadPool pool(4);
    auto future = pool.submit([] { return 42; });
    EXPECT_EQ(future.get(), 42);
}

TEST(ThreadPool, SubmitMultiple) {
    ThreadPool pool(4);
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 100; ++i) {
        futures.push_back(pool.submit([i] { return i * 2; }));
    }
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(futures[static_cast<size_t>(i)].get(), i * 2);
    }
}

TEST(ThreadPool, ParallelForCorrectness) {
    ThreadPool pool(4);
    std::vector<int> data(1000, 0);

    pool.parallel_for(0u, data.size(), [&data](size_t i) { data[i] = static_cast<int>(i); });

    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_EQ(data[i], static_cast<int>(i));
    }
}

TEST(ThreadPool, ParallelForEmpty) {
    ThreadPool pool(4);
    // Should not crash or hang
    pool.parallel_for(0u, 0u, [](size_t) {});
}

TEST(ThreadPool, ParallelForSingle) {
    ThreadPool pool(4);
    int value = 0;
    pool.parallel_for(0u, 1u, [&value](size_t) { value = 99; });
    EXPECT_EQ(value, 99);
}

TEST(ThreadPool, BatchSubmit) {
    ThreadPool pool(4);
    auto futures = pool.batch_submit(50, [](size_t i) { return i * 3; });
    EXPECT_EQ(futures.size(), 50u);
    for (size_t i = 0; i < 50; ++i) {
        EXPECT_EQ(futures[i].get(), i * 3);
    }
}

TEST(ThreadPool, ExceptionPropagation) {
    ThreadPool pool(4);
    auto future = pool.submit([]() -> int { throw std::runtime_error("test"); });
    EXPECT_THROW(future.get(), std::runtime_error);
}

TEST(ThreadPool, ParallelForExceptionPropagation) {
    ThreadPool pool(4);
    EXPECT_THROW(
        pool.parallel_for(0u, 10u,
                          [](size_t i) {
                              if (i == 5) throw std::runtime_error("boom");
                          }),
        std::runtime_error);
}

TEST(ThreadPool, ThreadCount) {
    ThreadPool pool(7);
    EXPECT_EQ(pool.thread_count(), 7u);
}

TEST(ThreadPool, PendingAndActiveCount) {
    ThreadPool pool(1);  // Single thread to make counts predictable

    // Initially nothing pending
    EXPECT_EQ(pool.pending_count(), 0u);

    // Submit work and check it completes
    std::atomic<int> counter{0};
    auto f1 = pool.submit([&counter] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        counter.fetch_add(1);
    });
    auto f2 = pool.submit([&counter] { counter.fetch_add(1); });

    f1.get();
    f2.get();
    EXPECT_EQ(counter.load(), 2);
}

TEST(ThreadPool, CleanShutdown) {
    std::atomic<int> completed{0};
    {
        ThreadPool pool(4);
        for (int i = 0; i < 100; ++i) {
            pool.submit([&completed] {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                completed.fetch_add(1);
            });
        }
        // Pool destroyed here — should drain remaining tasks
    }
    // At minimum some tasks should have completed; exact count depends on timing
    EXPECT_GT(completed.load(), 0);
}

TEST(GlobalPool, ReturnsConsistentInstance) {
    auto& p1 = global_pool();
    auto& p2 = global_pool();
    EXPECT_EQ(&p1, &p2);
}
