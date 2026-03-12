#include <gtest/gtest.h>

#include <chrono>
#include <string>

#include <eyetrack/comm/input_validation.hpp>

TEST(InputValidation, valid_frame_accepted) {
    auto result = eyetrack::InputValidator::validate_frame_dimensions(640, 480);
    EXPECT_TRUE(result.has_value());
}

TEST(InputValidation, frame_too_large_rejected) {
    auto result =
        eyetrack::InputValidator::validate_frame_size(11ULL * 1024 * 1024);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::InvalidInput);
}

TEST(InputValidation, frame_zero_dimensions_rejected) {
    auto result = eyetrack::InputValidator::validate_frame_dimensions(0, 0);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::InvalidInput);
}

TEST(InputValidation, frame_exceeds_max_dimensions) {
    auto result =
        eyetrack::InputValidator::validate_frame_dimensions(5000, 5000);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::InvalidInput);
}

TEST(InputValidation, timestamp_too_old_rejected) {
    auto old = std::chrono::steady_clock::now() - std::chrono::seconds(10);
    auto result = eyetrack::InputValidator::validate_timestamp(old);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::InvalidInput);
}

TEST(InputValidation, timestamp_fresh_accepted) {
    auto now = std::chrono::steady_clock::now();
    auto result = eyetrack::InputValidator::validate_timestamp(now);
    EXPECT_TRUE(result.has_value());
}

TEST(InputValidation, client_id_valid_format) {
    auto result = eyetrack::InputValidator::validate_client_id("user_123");
    EXPECT_TRUE(result.has_value());
}

TEST(InputValidation, client_id_invalid_chars) {
    auto result = eyetrack::InputValidator::validate_client_id("user@123!");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::InvalidInput);
}

TEST(InputValidation, client_id_too_long) {
    std::string long_id(65, 'a');
    auto result = eyetrack::InputValidator::validate_client_id(long_id);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::InvalidInput);
}

TEST(InputValidation, client_id_empty) {
    auto result = eyetrack::InputValidator::validate_client_id("");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::InvalidInput);
}

TEST(InputValidation, calibration_points_valid_range) {
    std::vector<eyetrack::Point2D> points(9, {0.5F, 0.5F});
    auto result = eyetrack::InputValidator::validate_calibration_points(points);
    EXPECT_TRUE(result.has_value());
}

TEST(InputValidation, calibration_too_few_points) {
    std::vector<eyetrack::Point2D> points(3, {0.5F, 0.5F});
    auto result = eyetrack::InputValidator::validate_calibration_points(points);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::InvalidInput);
}

TEST(InputValidation, calibration_too_many_points) {
    std::vector<eyetrack::Point2D> points(50, {0.5F, 0.5F});
    auto result = eyetrack::InputValidator::validate_calibration_points(points);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::InvalidInput);
}

TEST(InputValidation, calibration_point_out_of_range) {
    std::vector<eyetrack::Point2D> points = {
        {0.0F, 0.0F}, {0.5F, 0.5F}, {1.0F, 1.0F}, {1.5F, 0.5F}};
    auto result = eyetrack::InputValidator::validate_calibration_points(points);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::InvalidInput);
}
