#include <iris/utils/geometry_utils.hpp>

#include <cmath>
#include <numbers>

#include <gtest/gtest.h>

using namespace iris;

constexpr double kPi = std::numbers::pi;
constexpr double kTwoPi = 2.0 * kPi;

TEST(Cartesian2Polar, PointOnXAxis) {
    auto [rhos, phis] = cartesian2polar({5.0}, {0.0}, 0.0, 0.0);
    EXPECT_NEAR(rhos[0], 5.0, 1e-12);
    EXPECT_NEAR(phis[0], 0.0, 1e-12);
}

TEST(Cartesian2Polar, PointOnYAxis) {
    auto [rhos, phis] = cartesian2polar({0.0}, {3.0}, 0.0, 0.0);
    EXPECT_NEAR(rhos[0], 3.0, 1e-12);
    EXPECT_NEAR(phis[0], kPi / 2.0, 1e-12);
}

TEST(Cartesian2Polar, NegativeXAxis) {
    auto [rhos, phis] = cartesian2polar({-4.0}, {0.0}, 0.0, 0.0);
    EXPECT_NEAR(rhos[0], 4.0, 1e-12);
    EXPECT_NEAR(phis[0], kPi, 1e-12);
}

TEST(Cartesian2Polar, NegativeYAxis) {
    // atan2(-1, 0) = -pi/2, mapped to 3*pi/2
    auto [rhos, phis] = cartesian2polar({0.0}, {-2.0}, 0.0, 0.0);
    EXPECT_NEAR(rhos[0], 2.0, 1e-12);
    EXPECT_NEAR(phis[0], 3.0 * kPi / 2.0, 1e-12);
}

TEST(Cartesian2Polar, WithCenter) {
    // Point (10, 5) with center (7, 5) -> dx=3, dy=0 -> rho=3, phi=0
    auto [rhos, phis] = cartesian2polar({10.0}, {5.0}, 7.0, 5.0);
    EXPECT_NEAR(rhos[0], 3.0, 1e-12);
    EXPECT_NEAR(phis[0], 0.0, 1e-12);
}

TEST(Polar2Cartesian, BasicRoundTrip) {
    std::vector<double> xs = {1.0, 0.0, -1.0, 0.0};
    std::vector<double> ys = {0.0, 1.0, 0.0, -1.0};
    double cx = 5.0, cy = 3.0;

    // Shift by center
    for (auto& x : xs) x += cx;
    for (auto& y : ys) y += cy;

    auto [rhos, phis] = cartesian2polar(xs, ys, cx, cy);
    auto [xs2, ys2] = polar2cartesian(rhos, phis, cx, cy);

    for (size_t i = 0; i < xs.size(); ++i) {
        EXPECT_NEAR(xs2[i], xs[i], 1e-10) << "x mismatch at " << i;
        EXPECT_NEAR(ys2[i], ys[i], 1e-10) << "y mismatch at " << i;
    }
}

TEST(Cartesian2Polar, ContourOverload) {
    Contour c;
    c.points = {{3.0f, 0.0f}, {0.0f, 3.0f}};
    auto [rhos, phis] = cartesian2polar(c, 0.0, 0.0);
    ASSERT_EQ(rhos.size(), 2u);
    EXPECT_NEAR(rhos[0], 3.0, 1e-5);
    EXPECT_NEAR(phis[0], 0.0, 1e-5);
    EXPECT_NEAR(rhos[1], 3.0, 1e-5);
    EXPECT_NEAR(phis[1], kPi / 2.0, 1e-5);
}

TEST(Polar2Contour, Basic) {
    std::vector<double> rhos = {5.0, 5.0};
    std::vector<double> phis = {0.0, kPi / 2.0};
    auto contour = polar2contour(rhos, phis, 0.0, 0.0);
    ASSERT_EQ(contour.points.size(), 2u);
    EXPECT_NEAR(contour.points[0].x, 5.0f, 1e-4f);
    EXPECT_NEAR(contour.points[0].y, 0.0f, 1e-4f);
    EXPECT_NEAR(contour.points[1].x, 0.0f, 1e-4f);
    EXPECT_NEAR(contour.points[1].y, 5.0f, 1e-4f);
}

TEST(PolygonArea, Triangle) {
    Contour c;
    c.points = {{0.0f, 0.0f}, {4.0f, 0.0f}, {0.0f, 3.0f}};
    EXPECT_NEAR(polygon_area(c), 6.0, 1e-10);
}

TEST(PolygonArea, Square) {
    Contour c;
    c.points = {{0.0f, 0.0f}, {2.0f, 0.0f}, {2.0f, 2.0f}, {0.0f, 2.0f}};
    EXPECT_NEAR(polygon_area(c), 4.0, 1e-10);
}

TEST(PolygonArea, LessThanThreePoints) {
    Contour c;
    c.points = {{0.0f, 0.0f}, {1.0f, 1.0f}};
    EXPECT_DOUBLE_EQ(polygon_area(c), 0.0);
}

TEST(EstimateDiameter, Triangle) {
    Contour c;
    c.points = {{0.0f, 0.0f}, {3.0f, 0.0f}, {0.0f, 4.0f}};
    // Hypotenuse = 5
    EXPECT_NEAR(estimate_diameter(c), 5.0, 1e-5);
}

TEST(EstimateDiameter, SinglePoint) {
    Contour c;
    c.points = {{1.0f, 1.0f}};
    EXPECT_DOUBLE_EQ(estimate_diameter(c), 0.0);
}

TEST(FilledMask, EyeballCombinesAll) {
    GeometryMask gm;
    gm.pupil = cv::Mat::zeros(10, 10, CV_8UC1);
    gm.iris = cv::Mat::zeros(10, 10, CV_8UC1);
    gm.eyeball = cv::Mat::zeros(10, 10, CV_8UC1);

    gm.pupil.at<uint8_t>(5, 5) = 1;
    gm.iris.at<uint8_t>(3, 3) = 1;
    gm.eyeball.at<uint8_t>(1, 1) = 1;

    auto result = filled_eyeball_mask(gm);
    EXPECT_EQ(result.at<uint8_t>(5, 5), 1);
    EXPECT_EQ(result.at<uint8_t>(3, 3), 1);
    EXPECT_EQ(result.at<uint8_t>(1, 1), 1);
    EXPECT_EQ(result.at<uint8_t>(0, 0), 0);
}

TEST(ContourToFilledMask, Triangle) {
    Contour c;
    c.points = {{10.0f, 10.0f}, {20.0f, 10.0f}, {15.0f, 20.0f}};
    auto mask = contour_to_filled_mask(c, cv::Size(30, 30));
    EXPECT_EQ(mask.type(), CV_8UC1);
    EXPECT_EQ(mask.rows, 30);
    EXPECT_EQ(mask.cols, 30);
    // Centroid (15, 13) should be inside
    EXPECT_EQ(mask.at<uint8_t>(13, 15), 1);
    // Corner (0, 0) should be outside
    EXPECT_EQ(mask.at<uint8_t>(0, 0), 0);
}

TEST(ContourToFilledMask, TooFewPoints) {
    Contour c;
    c.points = {{0.0f, 0.0f}, {1.0f, 1.0f}};
    auto mask = contour_to_filled_mask(c, cv::Size(10, 10));
    EXPECT_EQ(cv::countNonZero(mask), 0);
}

TEST(PolygonLength, Square) {
    Contour c;
    c.points = {{0.0f, 0.0f}, {10.0f, 0.0f}, {10.0f, 10.0f}, {0.0f, 10.0f}};
    EXPECT_NEAR(polygon_length(c), 40.0, 1e-5);
}

TEST(PolygonLength, SinglePoint) {
    Contour c;
    c.points = {{5.0f, 5.0f}};
    EXPECT_DOUBLE_EQ(polygon_length(c), 0.0);
}

TEST(FilledMask, IrisCombinesPupilAndIris) {
    GeometryMask gm;
    gm.pupil = cv::Mat::zeros(10, 10, CV_8UC1);
    gm.iris = cv::Mat::zeros(10, 10, CV_8UC1);
    gm.eyeball = cv::Mat::zeros(10, 10, CV_8UC1);

    gm.pupil.at<uint8_t>(5, 5) = 1;
    gm.iris.at<uint8_t>(3, 3) = 1;
    gm.eyeball.at<uint8_t>(1, 1) = 1;

    auto result = filled_iris_mask(gm);
    EXPECT_EQ(result.at<uint8_t>(5, 5), 1);
    EXPECT_EQ(result.at<uint8_t>(3, 3), 1);
    EXPECT_EQ(result.at<uint8_t>(1, 1), 0);  // eyeball not included
}
