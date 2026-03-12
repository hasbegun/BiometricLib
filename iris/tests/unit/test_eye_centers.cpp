#include <iris/nodes/bisectors_eye_center.hpp>

#include <cmath>
#include <numbers>

#include <gtest/gtest.h>

using namespace iris;

/// Helper: create a circular contour centered at (cx, cy).
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

TEST(BisectorsEyeCenter, FindsCenterOfPerfectCircle) {
    GeometryPolygons polygons;
    polygons.iris = make_circle(200, 60.0f, 100.0f, 100.0f);
    polygons.pupil = make_circle(100, 25.0f, 100.0f, 100.0f);
    polygons.eyeball = make_circle(200, 90.0f, 100.0f, 100.0f);

    BisectorsEyeCenter estimator;
    auto result = estimator.run(polygons);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    auto& centers = result.value();
    EXPECT_NEAR(centers.iris_center.x, 100.0, 1.0);
    EXPECT_NEAR(centers.iris_center.y, 100.0, 1.0);
    EXPECT_NEAR(centers.pupil_center.x, 100.0, 1.0);
    EXPECT_NEAR(centers.pupil_center.y, 100.0, 1.0);
}

TEST(BisectorsEyeCenter, OffCenterPupil) {
    GeometryPolygons polygons;
    polygons.iris = make_circle(200, 60.0f, 100.0f, 100.0f);
    // Pupil offset to (105, 98)
    polygons.pupil = make_circle(100, 20.0f, 105.0f, 98.0f);
    polygons.eyeball = make_circle(200, 90.0f, 100.0f, 100.0f);

    BisectorsEyeCenter estimator;
    auto result = estimator.run(polygons);
    ASSERT_TRUE(result.has_value());

    auto& centers = result.value();
    EXPECT_NEAR(centers.iris_center.x, 100.0, 1.0);
    EXPECT_NEAR(centers.iris_center.y, 100.0, 1.0);
    EXPECT_NEAR(centers.pupil_center.x, 105.0, 1.0);
    EXPECT_NEAR(centers.pupil_center.y, 98.0, 1.0);
}

TEST(BisectorsEyeCenter, PartialArc) {
    // Half-circle (180 degrees) should still find reasonable center
    Contour half_iris;
    for (int i = 0; i < 100; ++i) {
        const double angle = std::numbers::pi * static_cast<double>(i) / 100.0;
        half_iris.points.emplace_back(
            100.0f + 60.0f * static_cast<float>(std::cos(angle)),
            100.0f + 60.0f * static_cast<float>(std::sin(angle)));
    }

    GeometryPolygons polygons;
    polygons.iris = half_iris;
    polygons.pupil = make_circle(50, 20.0f, 100.0f, 100.0f);
    polygons.eyeball = make_circle(200, 90.0f, 100.0f, 100.0f);

    BisectorsEyeCenter estimator;
    auto result = estimator.run(polygons);
    ASSERT_TRUE(result.has_value());

    // With half arc, center estimate should still be close
    EXPECT_NEAR(result.value().iris_center.x, 100.0, 5.0);
    EXPECT_NEAR(result.value().iris_center.y, 100.0, 5.0);
}

TEST(BisectorsEyeCenter, TooFewPointsFails) {
    GeometryPolygons polygons;
    polygons.iris.points = {{0.0f, 0.0f}, {1.0f, 0.0f}};  // Only 2 points
    polygons.pupil = polygons.iris;
    polygons.eyeball = polygons.iris;

    BisectorsEyeCenter estimator;
    auto result = estimator.run(polygons);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::GeometryEstimationFailed);
}

TEST(BisectorsEyeCenter, CustomParams) {
    GeometryPolygons polygons;
    polygons.iris = make_circle(200, 60.0f, 50.0f, 50.0f);
    polygons.pupil = make_circle(100, 25.0f, 50.0f, 50.0f);
    polygons.eyeball = make_circle(200, 90.0f, 50.0f, 50.0f);

    BisectorsEyeCenter estimator(BisectorsEyeCenter::Params{
        .num_bisectors = 50,
        .min_distance_between_sector_points = 0.5,
        .max_iterations = 20,
    });

    auto result = estimator.run(polygons);
    ASSERT_TRUE(result.has_value());

    EXPECT_NEAR(result.value().iris_center.x, 50.0, 2.0);
    EXPECT_NEAR(result.value().iris_center.y, 50.0, 2.0);
}
