#include <gtest/gtest.h>

#include <eyetrack/utils/image_utils.hpp>
#include <opencv2/imgproc.hpp>

namespace {

cv::Mat make_test_image(int width = 640, int height = 480, int type = CV_8UC3) {
    return cv::Mat(height, width, type, cv::Scalar(100, 150, 200));
}

}  // namespace

// --- safe_crop tests ---

TEST(ImageUtils, safe_crop_within_bounds) {
    auto image = make_test_image();
    cv::Rect roi(100, 50, 200, 150);

    auto result = eyetrack::safe_crop(image, roi);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_EQ(result->cols, 200);
    EXPECT_EQ(result->rows, 150);
}

TEST(ImageUtils, safe_crop_clipped_at_boundary) {
    auto image = make_test_image(640, 480);
    // ROI extends beyond the right and bottom edges
    cv::Rect roi(500, 400, 300, 200);

    auto result = eyetrack::safe_crop(image, roi);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Should be clipped to [500,640) x [400,480)
    EXPECT_EQ(result->cols, 140);
    EXPECT_EQ(result->rows, 80);
}

TEST(ImageUtils, safe_crop_entirely_outside) {
    auto image = make_test_image(640, 480);
    cv::Rect roi(700, 500, 100, 100);

    auto result = eyetrack::safe_crop(image, roi);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::InvalidInput);
}

TEST(ImageUtils, safe_crop_zero_size) {
    auto image = make_test_image();

    // Zero width
    auto result1 = eyetrack::safe_crop(image, cv::Rect(100, 100, 0, 50));
    ASSERT_FALSE(result1.has_value());
    EXPECT_EQ(result1.error().code, eyetrack::ErrorCode::InvalidInput);

    // Zero height
    auto result2 = eyetrack::safe_crop(image, cv::Rect(100, 100, 50, 0));
    ASSERT_FALSE(result2.has_value());
    EXPECT_EQ(result2.error().code, eyetrack::ErrorCode::InvalidInput);
}

// --- resolution_scale tests ---

TEST(ImageUtils, resolution_scale_down) {
    auto image = make_test_image(1280, 960);

    auto result = eyetrack::resolution_scale(image, 0.5);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_EQ(result->cols, 640);
    EXPECT_EQ(result->rows, 480);
}

TEST(ImageUtils, resolution_scale_up) {
    auto image = make_test_image(320, 240);

    auto result = eyetrack::resolution_scale(image, 2.0);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_EQ(result->cols, 640);
    EXPECT_EQ(result->rows, 480);
}

// --- format conversion tests ---

TEST(ImageUtils, format_bgr_to_rgb) {
    // Create a BGR image with known pixel value
    cv::Mat bgr(2, 2, CV_8UC3, cv::Scalar(10, 20, 30));  // B=10, G=20, R=30

    auto result = eyetrack::bgr_to_rgb(bgr);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_EQ(result->channels(), 3);
    auto pixel = result->at<cv::Vec3b>(0, 0);
    // After BGR->RGB: R=30, G=20, B=10
    EXPECT_EQ(pixel[0], 30);  // R
    EXPECT_EQ(pixel[1], 20);  // G
    EXPECT_EQ(pixel[2], 10);  // B
}

TEST(ImageUtils, format_grayscale_to_bgr) {
    cv::Mat gray(100, 100, CV_8UC1, cv::Scalar(128));

    auto result = eyetrack::grayscale_to_bgr(gray);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_EQ(result->channels(), 3);
    EXPECT_EQ(result->cols, 100);
    EXPECT_EQ(result->rows, 100);

    // Grayscale 128 -> BGR (128, 128, 128)
    auto pixel = result->at<cv::Vec3b>(0, 0);
    EXPECT_EQ(pixel[0], 128);
    EXPECT_EQ(pixel[1], 128);
    EXPECT_EQ(pixel[2], 128);
}

// --- empty input tests ---

TEST(ImageUtils, empty_input_returns_error) {
    cv::Mat empty;

    auto crop_result = eyetrack::safe_crop(empty, cv::Rect(0, 0, 10, 10));
    ASSERT_FALSE(crop_result.has_value());
    EXPECT_EQ(crop_result.error().code, eyetrack::ErrorCode::InvalidInput);

    auto scale_result = eyetrack::resolution_scale(empty, 1.0);
    ASSERT_FALSE(scale_result.has_value());
    EXPECT_EQ(scale_result.error().code, eyetrack::ErrorCode::InvalidInput);

    auto bgr_result = eyetrack::bgr_to_rgb(empty);
    ASSERT_FALSE(bgr_result.has_value());
    EXPECT_EQ(bgr_result.error().code, eyetrack::ErrorCode::InvalidInput);

    auto gray_result = eyetrack::grayscale_to_bgr(empty);
    ASSERT_FALSE(gray_result.has_value());
    EXPECT_EQ(gray_result.error().code, eyetrack::ErrorCode::InvalidInput);
}
