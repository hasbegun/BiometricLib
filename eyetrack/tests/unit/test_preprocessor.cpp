#include <gtest/gtest.h>

#include <eyetrack/core/node_registry.hpp>
#include <eyetrack/nodes/preprocessor.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace {

static const std::string kProjectRoot = EYETRACK_PROJECT_ROOT;
static const std::string kFixtureDir = kProjectRoot + "/tests/fixtures/face_images";

// Helper: create a synthetic BGR image
cv::Mat make_test_image(int width, int height, int channels = 3) {
    cv::Mat img(height, width, channels == 3 ? CV_8UC3 : CV_8UC1);
    cv::randu(img, cv::Scalar::all(30), cv::Scalar::all(220));
    return img;
}

// Helper: create a low-contrast grayscale image (narrow intensity range)
cv::Mat make_low_contrast_image(int width, int height) {
    cv::Mat img(height, width, CV_8UC3);
    cv::randu(img, cv::Scalar::all(100), cv::Scalar::all(130));
    return img;
}

}  // namespace

TEST(Preprocessor, resize_to_target_resolution) {
    eyetrack::Preprocessor::Config config;
    config.target_width = 640;
    config.target_height = 480;
    eyetrack::Preprocessor pp(config);

    auto input = make_test_image(1280, 960);
    auto result = pp.run(input);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_EQ(result->cols, 640);
    EXPECT_EQ(result->rows, 480);
}

TEST(Preprocessor, bgr_to_grayscale) {
    eyetrack::Preprocessor pp;

    auto input = make_test_image(640, 480, 3);
    ASSERT_EQ(input.channels(), 3);

    auto result = pp.run(input);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->channels(), 1);
}

TEST(Preprocessor, clahe_output_range) {
    eyetrack::Preprocessor pp;

    auto input = make_test_image(640, 480);
    auto result = pp.run(input);
    ASSERT_TRUE(result.has_value());

    double min_val = 0;
    double max_val = 0;
    cv::minMaxLoc(*result, &min_val, &max_val);
    EXPECT_GE(min_val, 0.0);
    EXPECT_LE(max_val, 255.0);
}

TEST(Preprocessor, clahe_increases_contrast) {
    eyetrack::Preprocessor pp;

    auto input = make_low_contrast_image(640, 480);

    // Compute stddev of grayscale input
    cv::Mat gray_input;
    cv::cvtColor(input, gray_input, cv::COLOR_BGR2GRAY);
    cv::Scalar mean_in, stddev_in;
    cv::meanStdDev(gray_input, mean_in, stddev_in);

    auto result = pp.run(input);
    ASSERT_TRUE(result.has_value());

    cv::Scalar mean_out, stddev_out;
    cv::meanStdDev(*result, mean_out, stddev_out);

    // CLAHE should increase contrast (stddev) of a low-contrast image
    EXPECT_GE(stddev_out[0], stddev_in[0])
        << "CLAHE should increase stddev. Input: " << stddev_in[0]
        << " Output: " << stddev_out[0];
}

TEST(Preprocessor, bilateral_denoise_optional) {
    eyetrack::Preprocessor::Config config_no_denoise;
    config_no_denoise.enable_denoise = false;
    eyetrack::Preprocessor pp_no_denoise(config_no_denoise);

    eyetrack::Preprocessor::Config config_denoise;
    config_denoise.enable_denoise = true;
    eyetrack::Preprocessor pp_denoise(config_denoise);

    auto input = make_test_image(640, 480);

    auto result_no = pp_no_denoise.run(input);
    auto result_yes = pp_denoise.run(input);
    ASSERT_TRUE(result_no.has_value());
    ASSERT_TRUE(result_yes.has_value());

    // With denoise enabled, output should differ from non-denoised
    // (bilateral filter smooths the image)
    EXPECT_FALSE(std::equal(result_no->begin<uint8_t>(), result_no->end<uint8_t>(),
                            result_yes->begin<uint8_t>()));
}

TEST(Preprocessor, empty_input_returns_error) {
    eyetrack::Preprocessor pp;

    cv::Mat empty;
    auto result = pp.run(empty);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::InvalidInput);
}

TEST(Preprocessor, preserves_image_content) {
    eyetrack::Preprocessor pp;

    // Load a fixture image
    auto input = cv::imread(kFixtureDir + "/frontal_01_standard.png");
    if (input.empty()) {
        GTEST_SKIP() << "Fixture image not found";
    }

    auto result = pp.run(input);
    ASSERT_TRUE(result.has_value());

    // Output should be non-zero and have reasonable pixel distribution
    cv::Scalar mean_val, stddev_val;
    cv::meanStdDev(*result, mean_val, stddev_val);
    EXPECT_GT(mean_val[0], 10.0) << "Output seems too dark";
    EXPECT_GT(stddev_val[0], 5.0) << "Output has near-zero variance";
}

TEST(Preprocessor, node_registry_lookup) {
    auto& registry = eyetrack::NodeRegistry::instance();
    EXPECT_TRUE(registry.has("eyetrack.Preprocessor"));

    auto factory = registry.lookup("eyetrack.Preprocessor");
    ASSERT_TRUE(factory.has_value());

    // Create via factory
    eyetrack::NodeParams params;
    auto node = factory.value()(params);
    EXPECT_TRUE(node.has_value());
}
