#include <iris/nodes/occlusion_calculator.hpp>

#include <cmath>
#include <numbers>

#include <gtest/gtest.h>

using namespace iris;

namespace {

Contour make_circle(float cx, float cy, float r, int n = 360) {
    Contour c;
    c.points.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        const double angle =
            static_cast<double>(i) * 2.0 * std::numbers::pi / static_cast<double>(n);
        c.points.emplace_back(cx + r * static_cast<float>(std::cos(angle)),
                              cy + r * static_cast<float>(std::sin(angle)));
    }
    return c;
}

}  // namespace

TEST(OcclusionCalculator, NoNoiseFullVisibility) {
    const int sz = 200;
    cv::Mat noise = cv::Mat::zeros(sz, sz, CV_8UC1);

    GeometryPolygons gp;
    gp.pupil = make_circle(100.0f, 100.0f, 20.0f);
    gp.iris = make_circle(100.0f, 100.0f, 50.0f);
    gp.eyeball = make_circle(100.0f, 100.0f, 80.0f);

    EyeOrientation orient{.angle = 0.0};
    EyeCenters centers{.pupil_center = {100, 100}, .iris_center = {100, 100}};
    NoiseMask nm{.mask = noise};

    OcclusionCalculator node;
    auto result = node.run(gp, nm, orient, centers);
    ASSERT_TRUE(result.has_value());
    // Should be close to 1.0 (fully visible)
    EXPECT_GT(result->fraction, 0.9);
}

TEST(OcclusionCalculator, HalfOccluded) {
    const int sz = 200;
    // Noise mask covering the top half
    cv::Mat noise = cv::Mat::zeros(sz, sz, CV_8UC1);
    noise(cv::Rect(0, 0, sz, sz / 2)) = 1;

    GeometryPolygons gp;
    gp.pupil = make_circle(100.0f, 100.0f, 20.0f);
    gp.iris = make_circle(100.0f, 100.0f, 50.0f);
    gp.eyeball = make_circle(100.0f, 100.0f, 80.0f);

    EyeOrientation orient{.angle = 0.0};
    EyeCenters centers{.pupil_center = {100, 100}, .iris_center = {100, 100}};
    NoiseMask nm{.mask = noise};

    OcclusionCalculator node;
    auto result = node.run(gp, nm, orient, centers);
    ASSERT_TRUE(result.has_value());
    // Should be around 0.5
    EXPECT_GT(result->fraction, 0.3);
    EXPECT_LT(result->fraction, 0.7);
}

TEST(OcclusionCalculator, ZeroQuantileAngle) {
    const int sz = 100;
    cv::Mat noise = cv::Mat::zeros(sz, sz, CV_8UC1);

    GeometryPolygons gp;
    gp.pupil = make_circle(50.0f, 50.0f, 10.0f);
    gp.iris = make_circle(50.0f, 50.0f, 30.0f);
    gp.eyeball = make_circle(50.0f, 50.0f, 45.0f);

    OcclusionCalculator::Params params;
    params.quantile_angle = 0.0;
    OcclusionCalculator node{params};

    EyeOrientation orient{.angle = 0.0};
    EyeCenters centers{.pupil_center = {50, 50}, .iris_center = {50, 50}};
    NoiseMask nm{.mask = noise};

    auto result = node.run(gp, nm, orient, centers);
    ASSERT_TRUE(result.has_value());
    EXPECT_NEAR(result->fraction, 0.0, 1e-10);
}

TEST(OcclusionCalculator, SmallerQuantileAngle) {
    const int sz = 200;
    cv::Mat noise = cv::Mat::zeros(sz, sz, CV_8UC1);

    GeometryPolygons gp;
    gp.pupil = make_circle(100.0f, 100.0f, 20.0f);
    gp.iris = make_circle(100.0f, 100.0f, 50.0f);
    gp.eyeball = make_circle(100.0f, 100.0f, 80.0f);

    OcclusionCalculator::Params params;
    params.quantile_angle = 30.0;
    OcclusionCalculator node{params};

    EyeOrientation orient{.angle = 0.0};
    EyeCenters centers{.pupil_center = {100, 100}, .iris_center = {100, 100}};
    NoiseMask nm{.mask = noise};

    auto result = node.run(gp, nm, orient, centers);
    ASSERT_TRUE(result.has_value());
    // Should still be high (no noise)
    EXPECT_GT(result->fraction, 0.8);
}

TEST(OcclusionCalculator, NodeParamsConstruction) {
    std::unordered_map<std::string, std::string> np;
    np["quantile_angle"] = "45.0";
    OcclusionCalculator node{np};

    const int sz = 200;
    cv::Mat noise = cv::Mat::zeros(sz, sz, CV_8UC1);

    GeometryPolygons gp;
    gp.pupil = make_circle(100.0f, 100.0f, 20.0f);
    gp.iris = make_circle(100.0f, 100.0f, 50.0f);
    gp.eyeball = make_circle(100.0f, 100.0f, 80.0f);

    EyeOrientation orient{.angle = 0.0};
    EyeCenters centers{.pupil_center = {100, 100}, .iris_center = {100, 100}};
    NoiseMask nm{.mask = noise};

    auto result = node.run(gp, nm, orient, centers);
    ASSERT_TRUE(result.has_value());
}
