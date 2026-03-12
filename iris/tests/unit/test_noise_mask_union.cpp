#include <iris/nodes/noise_mask_union.hpp>

#include <gtest/gtest.h>

using namespace iris;

TEST(NoiseMaskUnion, TwoNonOverlapping) {
    cv::Mat m1 = cv::Mat::zeros(10, 10, CV_8UC1);
    cv::Mat m2 = cv::Mat::zeros(10, 10, CV_8UC1);
    m1.at<uint8_t>(0, 0) = 1;
    m2.at<uint8_t>(9, 9) = 1;

    NoiseMaskUnion node;
    auto result = node.run({NoiseMask{m1}, NoiseMask{m2}});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->mask.at<uint8_t>(0, 0), 1);
    EXPECT_EQ(result->mask.at<uint8_t>(9, 9), 1);
    EXPECT_EQ(result->mask.at<uint8_t>(5, 5), 0);
}

TEST(NoiseMaskUnion, Overlapping) {
    cv::Mat m1 = cv::Mat::zeros(5, 5, CV_8UC1);
    cv::Mat m2 = cv::Mat::zeros(5, 5, CV_8UC1);
    m1.at<uint8_t>(2, 2) = 1;
    m2.at<uint8_t>(2, 2) = 1;

    NoiseMaskUnion node;
    auto result = node.run({NoiseMask{m1}, NoiseMask{m2}});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->mask.at<uint8_t>(2, 2), 1);
    EXPECT_EQ(cv::countNonZero(result->mask), 1);
}

TEST(NoiseMaskUnion, EmptyFails) {
    NoiseMaskUnion node;
    auto result = node.run({});
    EXPECT_FALSE(result.has_value());
}

TEST(NoiseMaskUnion, SizeMismatchFails) {
    cv::Mat m1 = cv::Mat::zeros(5, 5, CV_8UC1);
    cv::Mat m2 = cv::Mat::zeros(10, 10, CV_8UC1);

    NoiseMaskUnion node;
    auto result = node.run({NoiseMask{m1}, NoiseMask{m2}});
    EXPECT_FALSE(result.has_value());
}

TEST(NoiseMaskUnion, SingleMask) {
    cv::Mat m1 = cv::Mat::zeros(5, 5, CV_8UC1);
    m1.at<uint8_t>(3, 3) = 1;

    NoiseMaskUnion node;
    auto result = node.run({NoiseMask{m1}});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->mask.at<uint8_t>(3, 3), 1);
}
