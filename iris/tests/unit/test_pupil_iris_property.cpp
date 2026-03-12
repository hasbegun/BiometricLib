#include <iris/nodes/pupil_iris_property_calculator.hpp>

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

TEST(PupilIrisProperty, ConcentricCircles) {
    GeometryPolygons gp;
    gp.pupil = make_circle(200.0f, 200.0f, 30.0f);
    gp.iris = make_circle(200.0f, 200.0f, 100.0f);

    EyeCenters ec;
    ec.pupil_center = {200.0, 200.0};
    ec.iris_center = {200.0, 200.0};

    PupilIrisPropertyCalculator node;
    auto result = node.run(gp, ec);
    ASSERT_TRUE(result.has_value());

    // diameter_ratio ≈ 60/200 = 0.3
    EXPECT_NEAR(result->diameter_ratio, 0.3, 0.02);
    // center_distance_ratio = 0 (concentric)
    EXPECT_NEAR(result->center_distance_ratio, 0.0, 1e-5);
}

TEST(PupilIrisProperty, OffsetPupil) {
    GeometryPolygons gp;
    gp.pupil = make_circle(210.0f, 200.0f, 30.0f);
    gp.iris = make_circle(200.0f, 200.0f, 100.0f);

    EyeCenters ec;
    ec.pupil_center = {210.0, 200.0};
    ec.iris_center = {200.0, 200.0};

    PupilIrisPropertyCalculator node;
    auto result = node.run(gp, ec);
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->center_distance_ratio, 0.0);
}

TEST(PupilIrisProperty, PupilTooSmall) {
    GeometryPolygons gp;
    gp.pupil = make_circle(200.0f, 200.0f, 0.1f);
    gp.iris = make_circle(200.0f, 200.0f, 100.0f);

    EyeCenters ec;
    ec.pupil_center = {200.0, 200.0};
    ec.iris_center = {200.0, 200.0};

    PupilIrisPropertyCalculator node;
    auto result = node.run(gp, ec);
    EXPECT_FALSE(result.has_value());
}

TEST(PupilIrisProperty, IrisTooSmall) {
    GeometryPolygons gp;
    gp.pupil = make_circle(200.0f, 200.0f, 30.0f);
    gp.iris = make_circle(200.0f, 200.0f, 40.0f);

    EyeCenters ec;
    ec.pupil_center = {200.0, 200.0};
    ec.iris_center = {200.0, 200.0};

    PupilIrisPropertyCalculator::Params params;
    params.min_iris_diameter = 150.0;
    PupilIrisPropertyCalculator node{params};
    auto result = node.run(gp, ec);
    EXPECT_FALSE(result.has_value());
}

TEST(PupilIrisProperty, NodeParamsConstruction) {
    std::unordered_map<std::string, std::string> np;
    np["min_pupil_diameter"] = "5.0";
    np["min_iris_diameter"] = "100.0";
    PupilIrisPropertyCalculator node{np};

    GeometryPolygons gp;
    gp.pupil = make_circle(200.0f, 200.0f, 30.0f);
    gp.iris = make_circle(200.0f, 200.0f, 100.0f);

    EyeCenters ec;
    ec.pupil_center = {200.0, 200.0};
    ec.iris_center = {200.0, 200.0};

    auto result = node.run(gp, ec);
    ASSERT_TRUE(result.has_value());
}
