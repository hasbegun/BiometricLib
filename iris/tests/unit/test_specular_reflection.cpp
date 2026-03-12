#include <iris/nodes/specular_reflection_detector.hpp>

#include <gtest/gtest.h>

using namespace iris;

TEST(SpecularReflection, BasicThreshold) {
    IRImage img;
    img.data = cv::Mat(4, 4, CV_8UC1, cv::Scalar(100));
    // Set some pixels above default threshold (254)
    img.data.at<uint8_t>(0, 0) = 254;
    img.data.at<uint8_t>(1, 1) = 255;
    img.data.at<uint8_t>(2, 2) = 253;

    SpecularReflectionDetector detector;
    auto result = detector.run(img);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    auto& mask = result.value().mask;
    EXPECT_EQ(mask.at<uint8_t>(0, 0), 0);  // 254 is NOT > 254 (cv::threshold uses >)
    EXPECT_EQ(mask.at<uint8_t>(1, 1), 1);  // 255 > 254
    EXPECT_EQ(mask.at<uint8_t>(2, 2), 0);  // 253 <= 254
    EXPECT_EQ(mask.at<uint8_t>(3, 3), 0);  // 100 <= 254
}

TEST(SpecularReflection, CustomThreshold) {
    IRImage img;
    img.data = cv::Mat(2, 2, CV_8UC1, cv::Scalar(200));
    img.data.at<uint8_t>(0, 0) = 150;

    SpecularReflectionDetector detector(SpecularReflectionDetector::Params{.reflection_threshold = 180});
    auto result = detector.run(img);
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(result.value().mask.at<uint8_t>(0, 0), 0);  // 150 <= 180
    EXPECT_EQ(result.value().mask.at<uint8_t>(0, 1), 1);  // 200 > 180
}

TEST(SpecularReflection, EmptyImageFails) {
    IRImage img;
    SpecularReflectionDetector detector;
    auto result = detector.run(img);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ValidationFailed);
}

TEST(SpecularReflection, NodeParamsConstruction) {
    std::unordered_map<std::string, std::string> params;
    params["reflection_threshold"] = "200";
    SpecularReflectionDetector detector(params);

    IRImage img;
    img.data = cv::Mat(1, 1, CV_8UC1, cv::Scalar(210));
    auto result = detector.run(img);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().mask.at<uint8_t>(0, 0), 1);  // 210 > 200
}
