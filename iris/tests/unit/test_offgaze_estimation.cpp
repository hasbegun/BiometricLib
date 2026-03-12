#include <iris/nodes/eccentricity_offgaze_estimation.hpp>

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

Contour make_ellipse(float cx, float cy, float a, float b, int n = 360) {
    Contour c;
    c.points.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        const double angle =
            static_cast<double>(i) * 2.0 * std::numbers::pi / static_cast<double>(n);
        c.points.emplace_back(cx + a * static_cast<float>(std::cos(angle)),
                              cy + b * static_cast<float>(std::sin(angle)));
    }
    return c;
}

}  // namespace

TEST(EccentricityOffgaze, CirclesLowEccentricity) {
    GeometryPolygons gp;
    gp.pupil = make_circle(100.0f, 100.0f, 20.0f);
    gp.iris = make_circle(100.0f, 100.0f, 50.0f);

    EccentricityOffgazeEstimation node;
    auto result = node.run(gp);
    ASSERT_TRUE(result.has_value());
    // Circles have eccentricity near 0
    EXPECT_LT(result->score, 0.05);
}

TEST(EccentricityOffgaze, EllipseHigherEccentricity) {
    GeometryPolygons gp;
    gp.pupil = make_ellipse(100.0f, 100.0f, 30.0f, 10.0f);
    gp.iris = make_ellipse(100.0f, 100.0f, 60.0f, 20.0f);

    EccentricityOffgazeEstimation::Params params;
    params.eccentricity_method = "moments";
    params.assembling_method = "min";
    EccentricityOffgazeEstimation node{params};

    auto result = node.run(gp);
    ASSERT_TRUE(result.has_value());
    // Ellipses with 3:1 ratio should have significant eccentricity
    EXPECT_GT(result->score, 0.1);
}

TEST(EccentricityOffgaze, EllipseFitMethod) {
    GeometryPolygons gp;
    gp.pupil = make_ellipse(100.0f, 100.0f, 30.0f, 15.0f);
    gp.iris = make_ellipse(100.0f, 100.0f, 60.0f, 30.0f);

    EccentricityOffgazeEstimation::Params params;
    params.eccentricity_method = "ellipse_fit";
    params.assembling_method = "mean";
    EccentricityOffgazeEstimation node{params};

    auto result = node.run(gp);
    ASSERT_TRUE(result.has_value());
    // 2:1 ellipse: eccentricity = sqrt(1 - (1/2)^2) ≈ 0.866
    EXPECT_GT(result->score, 0.5);
}

TEST(EccentricityOffgaze, AssemblingMax) {
    GeometryPolygons gp;
    gp.pupil = make_circle(100.0f, 100.0f, 20.0f);
    gp.iris = make_ellipse(100.0f, 100.0f, 60.0f, 30.0f);

    EccentricityOffgazeEstimation::Params params;
    params.assembling_method = "max";
    EccentricityOffgazeEstimation node{params};

    auto result = node.run(gp);
    ASSERT_TRUE(result.has_value());
    // Max should pick the elliptical iris eccentricity
    EXPECT_GT(result->score, 0.05);
}

TEST(EccentricityOffgaze, EmptyContourFails) {
    GeometryPolygons gp;
    EccentricityOffgazeEstimation node;
    auto result = node.run(gp);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::GeometryEstimationFailed);
}

TEST(EccentricityOffgaze, NodeParamsConstruction) {
    std::unordered_map<std::string, std::string> np;
    np["eccentricity_method"] = "ellipse_fit";
    np["assembling_method"] = "only_iris";

    EccentricityOffgazeEstimation node{np};

    GeometryPolygons gp;
    gp.pupil = make_circle(100.0f, 100.0f, 20.0f);
    gp.iris = make_ellipse(100.0f, 100.0f, 60.0f, 20.0f);

    auto result = node.run(gp);
    ASSERT_TRUE(result.has_value());
    // only_iris with 3:1 ellipse should be high
    EXPECT_GT(result->score, 0.5);
}
