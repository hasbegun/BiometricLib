#include <gtest/gtest.h>

#include <cmath>

#include <eyetrack/utils/geometry_utils.hpp>

namespace {

using namespace eyetrack;

// ---- EuclideanDistance ----

TEST(EuclideanDistance, zero_distance) {
    EXPECT_FLOAT_EQ(euclidean_distance({0.0F, 0.0F}, {0.0F, 0.0F}), 0.0F);
}

TEST(EuclideanDistance, unit_distance) {
    EXPECT_NEAR(euclidean_distance({0.0F, 0.0F}, {1.0F, 0.0F}), 1.0, 1e-10);
}

TEST(EuclideanDistance, diagonal_distance) {
    // 3-4-5 triangle
    EXPECT_NEAR(euclidean_distance({0.0F, 0.0F}, {3.0F, 4.0F}), 5.0, 1e-10);
}

TEST(EuclideanDistance, negative_coordinates) {
    // (-1,-1) to (2,3): dx=3, dy=4, dist=5
    EXPECT_NEAR(euclidean_distance({-1.0F, -1.0F}, {2.0F, 3.0F}), 5.0, 1e-10);
}

// ---- ComputeEAR ----

TEST(ComputeEAR, open_eye_landmarks) {
    // Open eye: vertical gaps ~3, horizontal gap ~10
    //   p1(2,7)  p2(5,7)
    // p0(0,5)      p3(10,5)
    //   p5(2,3)  p4(5,3)
    std::array<Point2D, 6> eye = {{
        {0.0F, 5.0F},   // p0
        {2.0F, 7.0F},   // p1
        {5.0F, 7.0F},   // p2
        {10.0F, 5.0F},  // p3
        {5.0F, 3.0F},   // p4
        {2.0F, 3.0F},   // p5
    }};

    float ear = compute_ear(eye);
    // v1 = |p1-p5| = |(2,7)-(2,3)| = 4
    // v2 = |p2-p4| = |(5,7)-(5,3)| = 4
    // h  = |p0-p3| = |(0,5)-(10,5)| = 10
    // EAR = (4 + 4) / (2 * 10) = 0.4
    EXPECT_NEAR(ear, 0.4F, 1e-5F);
}

TEST(ComputeEAR, closed_eye_landmarks) {
    // Closed eye: vertical gaps ~1, horizontal gap ~10
    std::array<Point2D, 6> eye = {{
        {0.0F, 5.0F},    // p0
        {2.0F, 5.5F},    // p1
        {5.0F, 5.5F},    // p2
        {10.0F, 5.0F},   // p3
        {5.0F, 4.5F},    // p4
        {2.0F, 4.5F},    // p5
    }};

    float ear = compute_ear(eye);
    // v1 = |p1-p5| = |(2,5.5)-(2,4.5)| = 1
    // v2 = |p2-p4| = |(5,5.5)-(5,4.5)| = 1
    // h  = |p0-p3| = 10
    // EAR = (1 + 1) / (2 * 10) = 0.1
    EXPECT_NEAR(ear, 0.1F, 1e-5F);
}

TEST(ComputeEAR, fully_closed_zero_ear) {
    // All vertical distances zero
    std::array<Point2D, 6> eye = {{
        {0.0F, 5.0F},
        {2.0F, 5.0F},
        {5.0F, 5.0F},
        {10.0F, 5.0F},
        {5.0F, 5.0F},
        {2.0F, 5.0F},
    }};

    EXPECT_FLOAT_EQ(compute_ear(eye), 0.0F);
}

TEST(ComputeEAR, degenerate_horizontal_zero) {
    // Horizontal distance zero — should not divide by zero
    std::array<Point2D, 6> eye = {{
        {5.0F, 5.0F},
        {5.0F, 7.0F},
        {5.0F, 7.0F},
        {5.0F, 5.0F},
        {5.0F, 3.0F},
        {5.0F, 3.0F},
    }};

    float ear = compute_ear(eye);
    EXPECT_FLOAT_EQ(ear, 0.0F);
    EXPECT_FALSE(std::isnan(ear));
    EXPECT_FALSE(std::isinf(ear));
}

// ---- ComputeEARAverage ----

TEST(ComputeEARAverage, average_of_both_eyes) {
    EyeLandmarks lm;
    // Left eye: EAR = 0.4
    lm.left = {{
        {0.0F, 5.0F}, {2.0F, 7.0F}, {5.0F, 7.0F},
        {10.0F, 5.0F}, {5.0F, 3.0F}, {2.0F, 3.0F},
    }};
    // Right eye: EAR = 0.1
    lm.right = {{
        {0.0F, 5.0F}, {2.0F, 5.5F}, {5.0F, 5.5F},
        {10.0F, 5.0F}, {5.0F, 4.5F}, {2.0F, 4.5F},
    }};

    float avg = compute_ear_average(lm);
    // (0.4 + 0.1) / 2 = 0.25
    EXPECT_NEAR(avg, 0.25F, 1e-5F);
}

// ---- NormalizePoint ----

TEST(NormalizePoint, unit_square) {
    auto p = normalize_point({0.5F, 0.5F}, 1.0F, 1.0F);
    EXPECT_FLOAT_EQ(p.x, 0.5F);
    EXPECT_FLOAT_EQ(p.y, 0.5F);
}

TEST(NormalizePoint, scale_down) {
    auto p = normalize_point({320.0F, 240.0F}, 640.0F, 480.0F);
    EXPECT_FLOAT_EQ(p.x, 0.5F);
    EXPECT_FLOAT_EQ(p.y, 0.5F);
}

// ---- PolynomialFeatures ----

TEST(PolynomialFeatures, output_length_6) {
    auto features = polynomial_features(1.0F, 2.0F);
    EXPECT_EQ(features.size(), 6U);
}

TEST(PolynomialFeatures, known_values) {
    auto f = polynomial_features(2.0F, 3.0F);
    // [1, x, y, x^2, y^2, xy] = [1, 2, 3, 4, 9, 6]
    ASSERT_EQ(f.size(), 6U);
    EXPECT_FLOAT_EQ(f[0], 1.0F);
    EXPECT_FLOAT_EQ(f[1], 2.0F);
    EXPECT_FLOAT_EQ(f[2], 3.0F);
    EXPECT_FLOAT_EQ(f[3], 4.0F);
    EXPECT_FLOAT_EQ(f[4], 9.0F);
    EXPECT_FLOAT_EQ(f[5], 6.0F);
}

TEST(PolynomialFeatures, zero_input) {
    auto f = polynomial_features(0.0F, 0.0F);
    ASSERT_EQ(f.size(), 6U);
    EXPECT_FLOAT_EQ(f[0], 1.0F);
    for (size_t i = 1; i < 6; ++i) {
        EXPECT_FLOAT_EQ(f[i], 0.0F) << "Element " << i << " should be 0";
    }
}

}  // namespace
