#include <iris/nodes/segmentation_binarizer.hpp>

#include <gtest/gtest.h>

using namespace iris;

static SegmentationMap make_segmap(int rows, int cols,
                                   float eyeball, float iris_val,
                                   float pupil, float eyelashes) {
    SegmentationMap segmap;
    segmap.predictions = cv::Mat(rows, cols, CV_32FC4,
                                 cv::Scalar(eyeball, iris_val, pupil, eyelashes));
    return segmap;
}

TEST(SegmentationBinarizer, DefaultThresholds) {
    // All channels at 0.6 -> all above default 0.5 threshold
    auto segmap = make_segmap(10, 10, 0.6f, 0.6f, 0.6f, 0.6f);

    SegmentationBinarizer binarizer;
    auto result = binarizer.run(segmap);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    auto& [geometry, noise] = result.value();
    EXPECT_EQ(geometry.eyeball.at<uint8_t>(5, 5), 1);
    EXPECT_EQ(geometry.iris.at<uint8_t>(5, 5), 1);
    EXPECT_EQ(geometry.pupil.at<uint8_t>(5, 5), 1);
    EXPECT_EQ(noise.mask.at<uint8_t>(5, 5), 1);
}

TEST(SegmentationBinarizer, BelowThreshold) {
    // All channels at 0.3 -> all below default 0.5 threshold
    auto segmap = make_segmap(10, 10, 0.3f, 0.3f, 0.3f, 0.3f);

    SegmentationBinarizer binarizer;
    auto result = binarizer.run(segmap);
    ASSERT_TRUE(result.has_value());

    auto& [geometry, noise] = result.value();
    EXPECT_EQ(geometry.eyeball.at<uint8_t>(0, 0), 0);
    EXPECT_EQ(geometry.iris.at<uint8_t>(0, 0), 0);
    EXPECT_EQ(geometry.pupil.at<uint8_t>(0, 0), 0);
    EXPECT_EQ(noise.mask.at<uint8_t>(0, 0), 0);
}

TEST(SegmentationBinarizer, MixedValues) {
    SegmentationMap segmap;
    segmap.predictions = cv::Mat(1, 2, CV_32FC4);

    // Pixel 0: eyeball=0.8, iris=0.3, pupil=0.7, eyelashes=0.2
    segmap.predictions.at<cv::Vec4f>(0, 0) = {0.8f, 0.3f, 0.7f, 0.2f};
    // Pixel 1: eyeball=0.1, iris=0.9, pupil=0.4, eyelashes=0.6
    segmap.predictions.at<cv::Vec4f>(0, 1) = {0.1f, 0.9f, 0.4f, 0.6f};

    SegmentationBinarizer binarizer;
    auto result = binarizer.run(segmap);
    ASSERT_TRUE(result.has_value());

    auto& [geometry, noise] = result.value();

    // Pixel 0
    EXPECT_EQ(geometry.eyeball.at<uint8_t>(0, 0), 1);   // 0.8 >= 0.5
    EXPECT_EQ(geometry.iris.at<uint8_t>(0, 0), 0);      // 0.3 < 0.5
    EXPECT_EQ(geometry.pupil.at<uint8_t>(0, 0), 1);     // 0.7 >= 0.5
    EXPECT_EQ(noise.mask.at<uint8_t>(0, 0), 0);         // 0.2 < 0.5

    // Pixel 1
    EXPECT_EQ(geometry.eyeball.at<uint8_t>(0, 1), 0);   // 0.1 < 0.5
    EXPECT_EQ(geometry.iris.at<uint8_t>(0, 1), 1);      // 0.9 >= 0.5
    EXPECT_EQ(geometry.pupil.at<uint8_t>(0, 1), 0);     // 0.4 < 0.5
    EXPECT_EQ(noise.mask.at<uint8_t>(0, 1), 1);         // 0.6 >= 0.5
}

TEST(SegmentationBinarizer, CustomThresholds) {
    auto segmap = make_segmap(1, 1, 0.7f, 0.7f, 0.7f, 0.7f);

    SegmentationBinarizer binarizer(SegmentationBinarizer::Params{
        .eyeball_threshold = 0.6,
        .iris_threshold = 0.8,  // 0.7 < 0.8 -> should be 0
        .pupil_threshold = 0.7,
        .eyelashes_threshold = 0.5,
    });
    auto result = binarizer.run(segmap);
    ASSERT_TRUE(result.has_value());

    auto& [geometry, noise] = result.value();
    EXPECT_EQ(geometry.eyeball.at<uint8_t>(0, 0), 1);  // 0.7 >= 0.6
    EXPECT_EQ(geometry.iris.at<uint8_t>(0, 0), 0);     // 0.7 < 0.8
    EXPECT_EQ(geometry.pupil.at<uint8_t>(0, 0), 1);    // 0.7 >= 0.7
    EXPECT_EQ(noise.mask.at<uint8_t>(0, 0), 1);        // 0.7 >= 0.5
}

TEST(SegmentationBinarizer, EmptyInputFails) {
    SegmentationMap segmap;
    SegmentationBinarizer binarizer;
    auto result = binarizer.run(segmap);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::SegmentationFailed);
}
