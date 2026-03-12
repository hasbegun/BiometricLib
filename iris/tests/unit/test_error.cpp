#include <iris/core/error.hpp>

#include <gtest/gtest.h>

using namespace iris;

TEST(ErrorCode, NameReturnsCorrectString) {
    EXPECT_EQ(error_code_name(ErrorCode::Ok), "Ok");
    EXPECT_EQ(error_code_name(ErrorCode::Cancelled), "Cancelled");
    EXPECT_EQ(error_code_name(ErrorCode::KeyExpired), "KeyExpired");
    EXPECT_EQ(error_code_name(ErrorCode::ConfigInvalid), "ConfigInvalid");
}

TEST(MakeError, CreatesUnexpected) {
    auto err = make_error(ErrorCode::EncodingFailed, "test message", "detail info");
    Result<int> result = err;
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::EncodingFailed);
    EXPECT_EQ(result.error().message, "test message");
    EXPECT_EQ(result.error().detail, "detail info");
}

TEST(Result, HoldsValue) {
    Result<int> r = 42;
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(r.value(), 42);
}

TEST(Result, HoldsError) {
    Result<int> r = make_error(ErrorCode::IoFailed, "fail");
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code, ErrorCode::IoFailed);
}
