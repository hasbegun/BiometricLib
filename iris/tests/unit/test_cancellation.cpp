#include <iris/core/cancellation.hpp>

#include <atomic>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using namespace iris;

TEST(CancellationToken, InitiallyNotCancelled) {
    CancellationToken token;
    EXPECT_FALSE(token.is_cancelled());
}

TEST(CancellationToken, CancelSetsFlag) {
    CancellationToken token;
    token.cancel();
    EXPECT_TRUE(token.is_cancelled());
}

TEST(CancellationToken, DoubleCancelIsSafe) {
    CancellationToken token;
    token.cancel();
    token.cancel();
    EXPECT_TRUE(token.is_cancelled());
}

TEST(CancellationToken, ConcurrentCancelAndCheck) {
    CancellationToken token;
    std::atomic<bool> seen_cancelled{false};

    // Multiple threads checking and one cancelling
    std::vector<std::jthread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&token, &seen_cancelled] {
            for (int j = 0; j < 10000; ++j) {
                if (token.is_cancelled()) {
                    seen_cancelled.store(true, std::memory_order_relaxed);
                    return;
                }
            }
        });
    }

    // Give checkers a head start, then cancel
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    token.cancel();

    threads.clear();  // join all

    EXPECT_TRUE(token.is_cancelled());
}
