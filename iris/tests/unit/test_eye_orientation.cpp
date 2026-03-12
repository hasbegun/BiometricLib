#include <iris/nodes/moment_orientation.hpp>

#include <cmath>
#include <numbers>

#include <gtest/gtest.h>

using namespace iris;

/// Helper: create an ellipse contour.
static Contour make_ellipse(int n_points, float a, float b,
                             float cx, float cy, float rotation = 0.0f) {
    Contour c;
    c.points.reserve(static_cast<size_t>(n_points));
    const double cos_r = std::cos(static_cast<double>(rotation));
    const double sin_r = std::sin(static_cast<double>(rotation));

    for (int i = 0; i < n_points; ++i) {
        const double t = 2.0 * std::numbers::pi * static_cast<double>(i) /
                         static_cast<double>(n_points);
        const double lx = static_cast<double>(a) * std::cos(t);
        const double ly = static_cast<double>(b) * std::sin(t);
        // Apply rotation
        const double rx = lx * cos_r - ly * sin_r;
        const double ry = lx * sin_r + ly * cos_r;
        c.points.emplace_back(cx + static_cast<float>(rx),
                               cy + static_cast<float>(ry));
    }
    return c;
}

/// Helper: create a circle contour.
static Contour make_circle(int n_points, float radius, float cx, float cy) {
    return make_ellipse(n_points, radius, radius, cx, cy);
}

TEST(MomentOrientation, HorizontalEllipse) {
    // Horizontal ellipse (a > b) -> angle ~= 0
    GeometryPolygons polygons;
    polygons.eyeball = make_ellipse(200, 80.0f, 40.0f, 100.0f, 100.0f);
    polygons.iris = make_circle(100, 50.0f, 100.0f, 100.0f);
    polygons.pupil = make_circle(50, 20.0f, 100.0f, 100.0f);

    MomentOrientation estimator;
    auto result = estimator.run(polygons);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_NEAR(result.value().angle, 0.0, 0.05);
}

TEST(MomentOrientation, VerticalEllipse) {
    // Vertical ellipse (b > a) -> angle ~= pi/2 or -pi/2
    GeometryPolygons polygons;
    polygons.eyeball = make_ellipse(200, 40.0f, 80.0f, 100.0f, 100.0f);
    polygons.iris = make_circle(100, 50.0f, 100.0f, 100.0f);
    polygons.pupil = make_circle(50, 20.0f, 100.0f, 100.0f);

    MomentOrientation estimator;
    auto result = estimator.run(polygons);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Should be near +/- pi/2
    EXPECT_NEAR(std::abs(result.value().angle), std::numbers::pi / 2.0, 0.05);
}

TEST(MomentOrientation, RotatedEllipse) {
    // 30-degree rotation
    constexpr float angle_rad = static_cast<float>(std::numbers::pi / 6.0);
    GeometryPolygons polygons;
    polygons.eyeball = make_ellipse(200, 80.0f, 40.0f, 100.0f, 100.0f, angle_rad);
    polygons.iris = make_circle(100, 50.0f, 100.0f, 100.0f);
    polygons.pupil = make_circle(50, 20.0f, 100.0f, 100.0f);

    MomentOrientation estimator;
    auto result = estimator.run(polygons);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_NEAR(result.value().angle, static_cast<double>(angle_rad), 0.05);
}

TEST(MomentOrientation, CircularShapeRejected) {
    GeometryPolygons polygons;
    polygons.eyeball = make_circle(200, 60.0f, 100.0f, 100.0f);
    polygons.iris = make_circle(100, 50.0f, 100.0f, 100.0f);
    polygons.pupil = make_circle(50, 20.0f, 100.0f, 100.0f);

    MomentOrientation estimator;
    auto result = estimator.run(polygons);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::GeometryEstimationFailed);
}

TEST(MomentOrientation, TooFewPointsFails) {
    GeometryPolygons polygons;
    polygons.eyeball.points = {{0.0f, 0.0f}, {1.0f, 0.0f}};
    polygons.iris = make_circle(100, 50.0f, 100.0f, 100.0f);
    polygons.pupil = make_circle(50, 20.0f, 100.0f, 100.0f);

    MomentOrientation estimator;
    auto result = estimator.run(polygons);
    EXPECT_FALSE(result.has_value());
}
