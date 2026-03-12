#include <iris/nodes/contour_smoother.hpp>
#include <iris/nodes/ellipse_fitter.hpp>
#include <iris/nodes/fusion_extrapolator.hpp>
#include <iris/nodes/linear_extrapolator.hpp>
#include <iris/utils/geometry_utils.hpp>

#include <cmath>
#include <numbers>

#include <gtest/gtest.h>

using namespace iris;

/// Helper: create a circular contour.
static Contour make_circle(int n_points, float radius, float cx, float cy) {
    Contour c;
    c.points.reserve(static_cast<size_t>(n_points));
    for (int i = 0; i < n_points; ++i) {
        const double angle = 2.0 * std::numbers::pi * static_cast<double>(i) /
                             static_cast<double>(n_points);
        c.points.emplace_back(cx + radius * static_cast<float>(std::cos(angle)),
                               cy + radius * static_cast<float>(std::sin(angle)));
    }
    return c;
}

// ---- ContourSmoother tests ----

TEST(ContourSmoother, SmoothsCircularContour) {
    const float center = 100.0f;
    GeometryPolygons polygons;
    polygons.iris = make_circle(200, 60.0f, center, center);
    polygons.pupil = make_circle(100, 25.0f, center, center);
    polygons.eyeball = make_circle(200, 90.0f, center, center);

    EyeCenters centers;
    centers.iris_center = {center, center};
    centers.pupil_center = {center, center};

    ContourSmoother smoother;
    auto result = smoother.run(polygons, centers);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Smoothed contour should have points approximately at uniform angles
    EXPECT_GT(result.value().iris.points.size(), 100u);
    EXPECT_GT(result.value().pupil.points.size(), 50u);
    // Eyeball should be unchanged
    EXPECT_EQ(result.value().eyeball.points.size(),
              polygons.eyeball.points.size());
}

TEST(ContourSmoother, PreservesCircularShape) {
    const float center = 100.0f;
    const float radius = 50.0f;
    GeometryPolygons polygons;
    polygons.iris = make_circle(360, radius, center, center);
    polygons.pupil = make_circle(180, 20.0f, center, center);
    polygons.eyeball = make_circle(360, 80.0f, center, center);

    EyeCenters centers;
    centers.iris_center = {center, center};
    centers.pupil_center = {center, center};

    ContourSmoother smoother;
    auto result = smoother.run(polygons, centers);
    ASSERT_TRUE(result.has_value());

    // Check smoothed points are still approximately on a circle
    for (const auto& pt : result.value().iris.points) {
        double dx = static_cast<double>(pt.x - center);
        double dy = static_cast<double>(pt.y - center);
        double r = std::hypot(dx, dy);
        EXPECT_NEAR(r, radius, 2.0) << "Point (" << pt.x << ", " << pt.y
                                     << ") deviates from circle";
    }
}

TEST(ContourSmoother, EmptyIrisFails) {
    GeometryPolygons polygons;
    EyeCenters centers;
    centers.iris_center = {100, 100};
    centers.pupil_center = {100, 100};

    ContourSmoother smoother;
    auto result = smoother.run(polygons, centers);
    EXPECT_FALSE(result.has_value());
}

// ---- LinearExtrapolator tests ----

TEST(LinearExtrapolator, ExtrapolatesCircle) {
    const float center = 100.0f;
    GeometryPolygons polygons;
    polygons.iris = make_circle(200, 60.0f, center, center);
    polygons.pupil = make_circle(100, 25.0f, center, center);
    polygons.eyeball = make_circle(200, 90.0f, center, center);

    EyeCenters centers;
    centers.iris_center = {center, center};
    centers.pupil_center = {center, center};

    LinearExtrapolator extrapolator;
    auto result = extrapolator.run(polygons, centers);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // With dphi=0.9 degrees, expect ~400 points (360/0.9)
    EXPECT_GT(result.value().iris.points.size(), 300u);
    EXPECT_GT(result.value().pupil.points.size(), 300u);
}

TEST(LinearExtrapolator, PreservesRadius) {
    const float center = 100.0f;
    const float radius = 50.0f;
    GeometryPolygons polygons;
    polygons.iris = make_circle(100, radius, center, center);
    polygons.pupil = make_circle(50, 20.0f, center, center);
    polygons.eyeball = make_circle(100, 80.0f, center, center);

    EyeCenters centers;
    centers.iris_center = {center, center};
    centers.pupil_center = {center, center};

    LinearExtrapolator extrapolator;
    auto result = extrapolator.run(polygons, centers);
    ASSERT_TRUE(result.has_value());

    // All extrapolated points should be on the original circle
    for (const auto& pt : result.value().iris.points) {
        double dx = static_cast<double>(pt.x - center);
        double dy = static_cast<double>(pt.y - center);
        double r = std::hypot(dx, dy);
        EXPECT_NEAR(r, radius, 1.0);
    }
}

TEST(LinearExtrapolator, CustomDphi) {
    const float center = 100.0f;
    GeometryPolygons polygons;
    polygons.iris = make_circle(100, 60.0f, center, center);
    polygons.pupil = make_circle(50, 25.0f, center, center);
    polygons.eyeball = make_circle(100, 90.0f, center, center);

    EyeCenters centers;
    centers.iris_center = {center, center};
    centers.pupil_center = {center, center};

    // Large dphi = fewer points
    LinearExtrapolator extrapolator(LinearExtrapolator::Params{.dphi = 5.0});
    auto result = extrapolator.run(polygons, centers);
    ASSERT_TRUE(result.has_value());

    // With dphi=5.0, expect ~72 points (360/5)
    EXPECT_GT(result.value().iris.points.size(), 50u);
    EXPECT_LT(result.value().iris.points.size(), 100u);
}

TEST(LinearExtrapolator, TooFewPointsFails) {
    GeometryPolygons polygons;
    polygons.iris.points = {{0.0f, 0.0f}, {1.0f, 0.0f}};
    polygons.pupil = polygons.iris;
    polygons.eyeball = polygons.iris;

    EyeCenters centers;
    centers.iris_center = {0.5, 0.0};
    centers.pupil_center = {0.5, 0.0};

    LinearExtrapolator extrapolator;
    auto result = extrapolator.run(polygons, centers);
    EXPECT_FALSE(result.has_value());
}

// ---- EllipseFitter tests ----

TEST(EllipseFitter, FitsCircularContour) {
    const float center = 100.0f;
    GeometryPolygons polygons;
    polygons.iris = make_circle(200, 60.0f, center, center);
    polygons.pupil = make_circle(100, 25.0f, center, center);
    polygons.eyeball = make_circle(200, 90.0f, center, center);

    EllipseFitter fitter;
    auto result = fitter.run(polygons);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    ASSERT_TRUE(result.value().has_value());

    auto& fitted = result.value().value();
    EXPECT_GT(fitted.iris.points.size(), 100u);
    EXPECT_GT(fitted.pupil.points.size(), 100u);
}

TEST(EllipseFitter, RejectsPupilOutsideIris) {
    GeometryPolygons polygons;
    // Pupil larger than iris — validation should fail
    polygons.iris = make_circle(100, 30.0f, 100.0f, 100.0f);
    polygons.pupil = make_circle(100, 80.0f, 100.0f, 100.0f);
    polygons.eyeball = make_circle(100, 90.0f, 100.0f, 100.0f);

    EllipseFitter fitter;
    auto result = fitter.run(polygons);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result.value().has_value());
}

TEST(EllipseFitter, TooFewPointsFails) {
    GeometryPolygons polygons;
    polygons.iris.points = {{0.0f, 0.0f}, {1.0f, 0.0f}, {0.5f, 1.0f}};
    polygons.pupil = polygons.iris;
    polygons.eyeball = polygons.iris;

    EllipseFitter fitter;
    auto result = fitter.run(polygons);
    EXPECT_FALSE(result.has_value());
}

// ---- FusionExtrapolator tests ----

TEST(FusionExtrapolator, FusesCircleAndEllipse) {
    const float center = 100.0f;
    GeometryPolygons polygons;
    polygons.iris = make_circle(200, 60.0f, center, center);
    polygons.pupil = make_circle(100, 25.0f, center, center);
    polygons.eyeball = make_circle(200, 90.0f, center, center);

    EyeCenters centers;
    centers.iris_center = {center, center};
    centers.pupil_center = {center, center};

    FusionExtrapolator fuser;
    auto result = fuser.run(polygons, centers);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_GT(result.value().iris.points.size(), 100u);
    EXPECT_GT(result.value().pupil.points.size(), 100u);
}
