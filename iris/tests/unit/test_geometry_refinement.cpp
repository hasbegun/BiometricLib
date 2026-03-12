#include <iris/nodes/contour_interpolator.hpp>
#include <iris/nodes/eyeball_distance_filter.hpp>
#include <iris/utils/geometry_utils.hpp>

#include <cmath>
#include <numbers>

#include <opencv2/imgproc.hpp>

#include <gtest/gtest.h>

using namespace iris;

/// Helper: create a circular contour with N points centered at (cx, cy).
static Contour make_circle_contour(int n_points, float radius,
                                    float cx, float cy) {
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

// ---- ContourInterpolator tests ----

TEST(ContourInterpolator, InterpolatesLargeGaps) {
    // Create a square with 4 points — large gaps between them
    GeometryPolygons polygons;
    polygons.iris.points = {{0.0f, 0.0f}, {100.0f, 0.0f},
                            {100.0f, 100.0f}, {0.0f, 100.0f}};
    polygons.pupil.points = {{40.0f, 40.0f}, {60.0f, 40.0f},
                             {60.0f, 60.0f}, {40.0f, 60.0f}};
    polygons.eyeball = polygons.iris;

    ContourInterpolator interpolator(ContourInterpolator::Params{
        .max_distance_between_boundary_points = 0.1  // 10% of diameter
    });

    auto result = interpolator.run(polygons);
    ASSERT_TRUE(result.has_value());

    // Interpolated iris should have more points than original 4
    EXPECT_GT(result.value().iris.points.size(), 4u);
    // Original points should still be present
    EXPECT_GT(result.value().pupil.points.size(), 4u);
}

TEST(ContourInterpolator, NoInterpolationWhenDense) {
    // Dense circle — all points close together
    GeometryPolygons polygons;
    polygons.iris = make_circle_contour(360, 50.0f, 100.0f, 100.0f);
    polygons.pupil = make_circle_contour(180, 20.0f, 100.0f, 100.0f);
    polygons.eyeball = make_circle_contour(360, 80.0f, 100.0f, 100.0f);

    ContourInterpolator interpolator(ContourInterpolator::Params{
        .max_distance_between_boundary_points = 0.05
    });

    auto result = interpolator.run(polygons);
    ASSERT_TRUE(result.has_value());

    // With dense input, interpolation adds very few or no extra points
    // (circle circumference = 2*pi*50 ~= 314, 360 points means ~0.87px spacing)
    // Iris diameter ~= 100, threshold = 0.05 * 100 = 5px
    // No gaps > 5px, so no interpolation needed
    EXPECT_EQ(result.value().iris.points.size(), 360u);
}

TEST(ContourInterpolator, EmptyIrisFails) {
    GeometryPolygons polygons;
    ContourInterpolator interpolator;
    auto result = interpolator.run(polygons);
    EXPECT_FALSE(result.has_value());
}

// ---- EyeballDistanceFilter tests ----

TEST(EyeballDistanceFilter, RemovesPointsNearNoise) {
    const int img_size = 200;
    const float center = 100.0f;

    GeometryPolygons polygons;
    polygons.iris = make_circle_contour(360, 60.0f, center, center);
    polygons.pupil = make_circle_contour(180, 25.0f, center, center);
    polygons.eyeball = make_circle_contour(360, 90.0f, center, center);

    // Create noise mask with a blob near the top
    NoiseMask noise;
    noise.mask = cv::Mat::zeros(img_size, img_size, CV_8UC1);
    cv::circle(noise.mask, cv::Point(100, 40), 10, cv::Scalar(1), cv::FILLED);

    EyeballDistanceFilter filter(EyeballDistanceFilter::Params{
        .min_distance_to_noise_and_eyeball = 0.05  // 5% of iris diameter
    });

    auto result = filter.run(polygons, noise);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Some iris points near the noise blob should be removed
    EXPECT_LT(result.value().iris.points.size(), polygons.iris.points.size());
    // Eyeball passes through unchanged
    EXPECT_EQ(result.value().eyeball.points.size(), polygons.eyeball.points.size());
}

TEST(EyeballDistanceFilter, NoNoisePassesThrough) {
    GeometryPolygons polygons;
    polygons.iris = make_circle_contour(100, 50.0f, 100.0f, 100.0f);
    polygons.pupil = make_circle_contour(50, 20.0f, 100.0f, 100.0f);
    polygons.eyeball = make_circle_contour(100, 80.0f, 100.0f, 100.0f);

    NoiseMask noise;
    noise.mask = cv::Mat::zeros(200, 200, CV_8UC1);

    EyeballDistanceFilter filter(EyeballDistanceFilter::Params{
        .min_distance_to_noise_and_eyeball = 0.005
    });

    auto result = filter.run(polygons, noise);
    ASSERT_TRUE(result.has_value());

    // With only eyeball contour as "noise", some iris points near the
    // eyeball boundary may be filtered, but most should survive
    EXPECT_GT(result.value().iris.points.size(), 50u);
}
