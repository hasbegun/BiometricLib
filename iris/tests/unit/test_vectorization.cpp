#include <iris/nodes/contour_extractor.hpp>
#include <iris/utils/geometry_utils.hpp>

#include <opencv2/imgproc.hpp>

#include <gtest/gtest.h>

using namespace iris;

/// Helper: create a GeometryMask with filled circles at known positions.
static GeometryMask make_circle_masks(int img_size, int eyeball_r,
                                       int iris_r, int pupil_r) {
    cv::Point center(img_size / 2, img_size / 2);
    GeometryMask gm;
    gm.eyeball = cv::Mat::zeros(img_size, img_size, CV_8UC1);
    gm.iris = cv::Mat::zeros(img_size, img_size, CV_8UC1);
    gm.pupil = cv::Mat::zeros(img_size, img_size, CV_8UC1);

    cv::circle(gm.eyeball, center, eyeball_r, cv::Scalar(1), cv::FILLED);
    cv::circle(gm.iris, center, iris_r, cv::Scalar(1), cv::FILLED);
    cv::circle(gm.pupil, center, pupil_r, cv::Scalar(1), cv::FILLED);

    return gm;
}

TEST(ContourExtractor, ExtractsCircularContours) {
    auto gm = make_circle_masks(200, 90, 60, 30);

    ContourExtractor extractor;
    auto result = extractor.run(gm);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    auto& polygons = result.value();
    // All three contours should have points
    EXPECT_GT(polygons.eyeball.points.size(), 10u);
    EXPECT_GT(polygons.iris.points.size(), 10u);
    EXPECT_GT(polygons.pupil.points.size(), 10u);
}

TEST(ContourExtractor, EmptyIrisFails) {
    GeometryMask gm;
    gm.eyeball = cv::Mat::zeros(100, 100, CV_8UC1);
    gm.iris = cv::Mat::zeros(100, 100, CV_8UC1);
    gm.pupil = cv::Mat::zeros(100, 100, CV_8UC1);

    ContourExtractor extractor;
    auto result = extractor.run(gm);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::VectorizationFailed);
}

TEST(ContourExtractor, AreaFilterRemovesSmallNoise) {
    auto gm = make_circle_masks(200, 90, 60, 30);

    // Add small noise blobs to iris mask
    cv::circle(gm.iris, cv::Point(10, 10), 3, cv::Scalar(1), cv::FILLED);
    cv::circle(gm.iris, cv::Point(190, 190), 2, cv::Scalar(1), cv::FILLED);

    ContourExtractor extractor;
    auto result = extractor.run(gm);
    ASSERT_TRUE(result.has_value());

    // Should still extract the main iris contour, ignoring noise
    EXPECT_GT(result.value().iris.points.size(), 10u);
}

TEST(ContourExtractor, ContoursApproximateCircle) {
    auto gm = make_circle_masks(300, 120, 80, 40);

    ContourExtractor extractor;
    auto result = extractor.run(gm);
    ASSERT_TRUE(result.has_value());

    // Iris contour should approximately match the expected area
    double expected_area = 3.14159 * 80 * 80;  // pi * r^2
    // The filled iris mask = pupil | iris, so contour area should match
    double contour_area = polygon_area(result.value().iris);
    EXPECT_NEAR(contour_area, expected_area, expected_area * 0.15);
}
