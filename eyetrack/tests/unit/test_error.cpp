#include <gtest/gtest.h>

#include <set>
#include <sstream>
#include <string>
#include <utility>

#include <eyetrack/core/error.hpp>

namespace {

using namespace eyetrack;

// ---- ErrorCode ----

TEST(ErrorCode, success_is_zero) {
    EXPECT_EQ(static_cast<uint16_t>(ErrorCode::Success), 0);
}

TEST(ErrorCode, all_codes_distinct) {
    std::set<uint16_t> values;
    values.insert(static_cast<uint16_t>(ErrorCode::Success));
    values.insert(static_cast<uint16_t>(ErrorCode::FaceNotDetected));
    values.insert(static_cast<uint16_t>(ErrorCode::EyeNotDetected));
    values.insert(static_cast<uint16_t>(ErrorCode::PupilNotDetected));
    values.insert(static_cast<uint16_t>(ErrorCode::CalibrationFailed));
    values.insert(static_cast<uint16_t>(ErrorCode::CalibrationNotLoaded));
    values.insert(static_cast<uint16_t>(ErrorCode::ModelLoadFailed));
    values.insert(static_cast<uint16_t>(ErrorCode::ConfigInvalid));
    values.insert(static_cast<uint16_t>(ErrorCode::ConnectionFailed));
    values.insert(static_cast<uint16_t>(ErrorCode::Timeout));
    values.insert(static_cast<uint16_t>(ErrorCode::Cancelled));
    values.insert(static_cast<uint16_t>(ErrorCode::Unauthenticated));
    values.insert(static_cast<uint16_t>(ErrorCode::PermissionDenied));
    values.insert(static_cast<uint16_t>(ErrorCode::InvalidInput));
    values.insert(static_cast<uint16_t>(ErrorCode::RateLimited));

    EXPECT_EQ(values.size(), 15U) << "All 15 ErrorCode values must be distinct";
}

// ---- EyetrackError ----

TEST(EyetrackError, construction_with_code_and_message) {
    EyetrackError err{ErrorCode::FaceNotDetected, "no face in frame", "frame 42"};
    EXPECT_EQ(err.code, ErrorCode::FaceNotDetected);
    EXPECT_EQ(err.message, "no face in frame");
    EXPECT_EQ(err.detail, "frame 42");
}

TEST(EyetrackError, default_message_is_empty) {
    EyetrackError err;
    EXPECT_EQ(err.code, ErrorCode::Success);
    EXPECT_TRUE(err.message.empty());
    EXPECT_TRUE(err.detail.empty());
}

TEST(EyetrackError, stream_operator) {
    EyetrackError err{ErrorCode::ModelLoadFailed, "file not found", "/models/face.onnx"};
    std::ostringstream oss;
    oss << err;
    std::string output = oss.str();

    EXPECT_NE(output.find("ModelLoadFailed"), std::string::npos);
    EXPECT_NE(output.find("file not found"), std::string::npos);
    EXPECT_NE(output.find("/models/face.onnx"), std::string::npos);
}

// ---- Result<T> ----

TEST(Result, success_value) {
    Result<int> result{42};
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 42);
}

TEST(Result, error_value) {
    Result<int> result = make_error(ErrorCode::PupilNotDetected, "threshold too high");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::PupilNotDetected);
    EXPECT_EQ(result.error().message, "threshold too high");
}

TEST(Result, move_semantics) {
    Result<std::string> result{std::string("hello world")};
    ASSERT_TRUE(result.has_value());

    Result<std::string> moved = std::move(result);
    ASSERT_TRUE(moved.has_value());
    EXPECT_EQ(moved.value(), "hello world");
}

TEST(Result, void_result) {
    // Success case
    Result<void> ok{};
    EXPECT_TRUE(ok.has_value());

    // Error case
    Result<void> err = make_error(ErrorCode::Timeout, "connection timed out");
    EXPECT_FALSE(err.has_value());
    EXPECT_EQ(err.error().code, ErrorCode::Timeout);
}

// ---- error_code_name ----

TEST(ErrorCodeName, all_codes_have_names) {
    EXPECT_EQ(error_code_name(ErrorCode::Success), "Success");
    EXPECT_EQ(error_code_name(ErrorCode::FaceNotDetected), "FaceNotDetected");
    EXPECT_EQ(error_code_name(ErrorCode::EyeNotDetected), "EyeNotDetected");
    EXPECT_EQ(error_code_name(ErrorCode::PupilNotDetected), "PupilNotDetected");
    EXPECT_EQ(error_code_name(ErrorCode::CalibrationFailed), "CalibrationFailed");
    EXPECT_EQ(error_code_name(ErrorCode::CalibrationNotLoaded), "CalibrationNotLoaded");
    EXPECT_EQ(error_code_name(ErrorCode::ModelLoadFailed), "ModelLoadFailed");
    EXPECT_EQ(error_code_name(ErrorCode::ConfigInvalid), "ConfigInvalid");
    EXPECT_EQ(error_code_name(ErrorCode::ConnectionFailed), "ConnectionFailed");
    EXPECT_EQ(error_code_name(ErrorCode::Timeout), "Timeout");
    EXPECT_EQ(error_code_name(ErrorCode::Cancelled), "Cancelled");
    EXPECT_EQ(error_code_name(ErrorCode::Unauthenticated), "Unauthenticated");
    EXPECT_EQ(error_code_name(ErrorCode::PermissionDenied), "PermissionDenied");
    EXPECT_EQ(error_code_name(ErrorCode::InvalidInput), "InvalidInput");
    EXPECT_EQ(error_code_name(ErrorCode::RateLimited), "RateLimited");
}

}  // namespace
