#include <gtest/gtest.h>

#include <eyetrack/comm/auth_middleware.hpp>

TEST(AuthMiddleware, noop_accepts_all) {
    eyetrack::NoOpAuthMiddleware auth;
    auto result = auth.authenticate("some_token");
    ASSERT_TRUE(result.has_value()) << result.error().message;
}

TEST(AuthMiddleware, noop_returns_client_identity) {
    eyetrack::NoOpAuthMiddleware auth;
    auto result = auth.authenticate("user_42");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().client_id, "user_42");
    EXPECT_EQ(result.value().role, "user");
}

TEST(AuthMiddleware, noop_empty_token_returns_anonymous) {
    eyetrack::NoOpAuthMiddleware auth;
    auto result = auth.authenticate("");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().client_id, "anonymous");
}

TEST(AuthMiddleware, interface_defined) {
    // Verify the interface compiles and factory works
    auto auth = eyetrack::make_default_auth();
    ASSERT_NE(auth, nullptr);
    auto result = auth->authenticate("test");
    EXPECT_TRUE(result.has_value());
}
