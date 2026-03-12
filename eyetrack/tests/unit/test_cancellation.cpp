#include <gtest/gtest.h>

#include <thread>

#include <eyetrack/core/cancellation.hpp>

namespace {

using namespace eyetrack;

TEST(CancellationToken, initially_not_cancelled) {
    CancellationToken token;
    EXPECT_FALSE(token.is_cancelled());
}

TEST(CancellationToken, cancel_sets_flag) {
    CancellationToken token;
    token.cancel();
    EXPECT_TRUE(token.is_cancelled());
}

TEST(CancellationToken, multiple_cancel_calls_idempotent) {
    CancellationToken token;
    token.cancel();
    token.cancel();
    token.cancel();
    EXPECT_TRUE(token.is_cancelled());
}

TEST(CancellationToken, shared_between_threads) {
    CancellationToken token;

    std::thread t([&token]() {
        token.cancel();
    });

    t.join();
    EXPECT_TRUE(token.is_cancelled());
}

}  // namespace
