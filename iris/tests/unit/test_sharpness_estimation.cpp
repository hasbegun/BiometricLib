#include <iris/nodes/sharpness_estimation.hpp>

#include <gtest/gtest.h>

using namespace iris;

TEST(SharpnessEstimation, SharpImageHighScore) {
    // High-frequency content: alternating black/white stripes
    cv::Mat image(64, 128, CV_64FC1);
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 128; ++x) {
            image.at<double>(y, x) = (x % 4 < 2) ? 1.0 : 0.0;
        }
    }
    cv::Mat mask = cv::Mat::ones(64, 128, CV_8UC1);

    NormalizedIris ni{.normalized_image = image, .normalized_mask = mask};

    SharpnessEstimation::Params params;
    params.lap_ksize = 3;
    params.erosion_ksize_w = 3;
    params.erosion_ksize_h = 3;
    SharpnessEstimation node{params};

    auto result = node.run(ni);
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->score, 0.0);
}

TEST(SharpnessEstimation, BlurredImageLowerScore) {
    // Uniform image: no edges -> low sharpness
    cv::Mat image(64, 128, CV_64FC1, cv::Scalar(0.5));
    cv::Mat mask = cv::Mat::ones(64, 128, CV_8UC1);

    NormalizedIris ni{.normalized_image = image, .normalized_mask = mask};

    SharpnessEstimation::Params params;
    params.lap_ksize = 3;
    params.erosion_ksize_w = 3;
    params.erosion_ksize_h = 3;
    SharpnessEstimation node{params};

    auto result = node.run(ni);
    ASSERT_TRUE(result.has_value());
    EXPECT_NEAR(result->score, 0.0, 1e-5);
}

TEST(SharpnessEstimation, AllMaskedZeroScore) {
    cv::Mat image(64, 128, CV_64FC1, cv::Scalar(0.5));
    cv::Mat mask = cv::Mat::zeros(64, 128, CV_8UC1);

    NormalizedIris ni{.normalized_image = image, .normalized_mask = mask};

    SharpnessEstimation node;
    auto result = node.run(ni);
    ASSERT_TRUE(result.has_value());
    EXPECT_NEAR(result->score, 0.0, 1e-10);
}

TEST(SharpnessEstimation, EmptyImageFails) {
    NormalizedIris ni;
    SharpnessEstimation node;
    auto result = node.run(ni);
    EXPECT_FALSE(result.has_value());
}
