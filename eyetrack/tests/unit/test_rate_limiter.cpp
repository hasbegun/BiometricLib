#include <gtest/gtest.h>

#include <thread>
#include <vector>

#include <eyetrack/comm/rate_limiter.hpp>

TEST(RateLimiter, allows_under_limit) {
    eyetrack::RateLimiter limiter(60);
    int allowed = 0;
    for (int i = 0; i < 30; ++i) {
        if (limiter.allow("client_a")) ++allowed;
    }
    EXPECT_EQ(allowed, 30);
}

TEST(RateLimiter, rejects_over_limit) {
    eyetrack::RateLimiter limiter(60);
    int allowed = 0;
    for (int i = 0; i < 65; ++i) {
        if (limiter.allow("client_a")) ++allowed;
    }
    // Should allow exactly 60 (initial bucket), reject the rest
    EXPECT_EQ(allowed, 60);
}

TEST(RateLimiter, token_bucket_refills) {
    eyetrack::RateLimiter limiter(60);
    // Exhaust tokens
    for (int i = 0; i < 60; ++i) {
        (void)limiter.allow("client_a");
    }
    EXPECT_FALSE(limiter.allow("client_a"));

    // Wait for refill (at 60/sec, 100ms should add ~6 tokens)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(limiter.allow("client_a"));
}

TEST(RateLimiter, per_client_isolation) {
    eyetrack::RateLimiter limiter(10);
    // Exhaust client_a
    for (int i = 0; i < 10; ++i) {
        (void)limiter.allow("client_a");
    }
    EXPECT_FALSE(limiter.allow("client_a"));

    // client_b should still be allowed
    EXPECT_TRUE(limiter.allow("client_b"));
}

TEST(RateLimiter, configurable_limit) {
    eyetrack::RateLimiter limiter(10);
    EXPECT_EQ(limiter.limit(), 10U);

    int allowed = 0;
    for (int i = 0; i < 15; ++i) {
        if (limiter.allow("client_a")) ++allowed;
    }
    EXPECT_EQ(allowed, 10);
}

TEST(RateLimiter, logs_warning_on_rate_limit) {
    // Just verify it doesn't crash — log output is tested via spdlog internals
    eyetrack::RateLimiter limiter(1);
    (void)limiter.allow("client_a");  // Use the one token
    EXPECT_FALSE(limiter.allow("client_a"));  // This should log a warning
}

TEST(RateLimiter, concurrent_access) {
    eyetrack::RateLimiter limiter(1000);
    constexpr int kThreads = 8;
    constexpr int kOpsPerThread = 100;

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&limiter, t] {
            for (int i = 0; i < kOpsPerThread; ++i) {
                (void)limiter.allow("client_" + std::to_string(t));
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }
    // No crash or data race
    SUCCEED();
}
