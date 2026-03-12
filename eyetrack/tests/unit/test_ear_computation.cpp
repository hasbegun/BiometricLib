#include <gtest/gtest.h>

#include <array>
#include <cmath>

#include <eyetrack/core/types.hpp>
#include <eyetrack/utils/geometry_utils.hpp>

using eyetrack::Point2D;
using eyetrack::EyeLandmarks;
using eyetrack::compute_ear;
using eyetrack::compute_ear_average;
using eyetrack::euclidean_distance;

namespace {

// Construct a 6-point eye with given vertical opening.
// Layout: p0=left corner, p1=upper-left, p2=upper-right,
//         p3=right corner, p4=lower-right, p5=lower-left
// Horizontal axis: p0 to p3 at y=0, separated by `width`.
// Vertical: p1/p2 at +half_height, p4/p5 at -half_height.
std::array<Point2D, 6> make_eye(float width, float half_height) {
    float hw = width / 2.0F;
    return {{
        {-hw, 0.0F},                    // p0: left corner
        {-hw / 2.0F, half_height},       // p1: upper-left
        {hw / 2.0F, half_height},        // p2: upper-right
        {hw, 0.0F},                      // p3: right corner
        {hw / 2.0F, -half_height},       // p4: lower-right
        {-hw / 2.0F, -half_height},      // p5: lower-left
    }};
}

}  // namespace

TEST(EARComputation, open_eye_ear_approximately_0_3) {
    // Open eye: width=30, half_height=4.5
    // v1 = dist(p1, p5) = dist((-7.5,4.5), (-7.5,-4.5)) = 9.0
    // v2 = dist(p2, p4) = dist((7.5,4.5), (7.5,-4.5)) = 9.0
    // h = dist(p0, p3) = dist((-15,0), (15,0)) = 30.0
    // EAR = (9+9)/(2*30) = 0.3
    auto eye = make_eye(30.0F, 4.5F);
    float ear = compute_ear(eye);
    EXPECT_NEAR(ear, 0.3F, 0.05F);
}

TEST(EARComputation, closed_eye_ear_approximately_0_1) {
    // Nearly closed: width=30, half_height=1.5
    // v1 = v2 = 3.0, h = 30.0
    // EAR = (3+3)/(2*30) = 0.1
    auto eye = make_eye(30.0F, 1.5F);
    float ear = compute_ear(eye);
    EXPECT_NEAR(ear, 0.1F, 0.05F);
}

TEST(EARComputation, fully_closed_ear_zero) {
    // All vertical distances zero: half_height = 0
    auto eye = make_eye(30.0F, 0.0F);
    float ear = compute_ear(eye);
    EXPECT_FLOAT_EQ(ear, 0.0F);
}

TEST(EARComputation, symmetric_eye_ear_consistent) {
    auto left = make_eye(30.0F, 4.5F);
    // Mirror horizontally for right eye
    std::array<Point2D, 6> right;
    for (size_t i = 0; i < 6; ++i) {
        right[i] = {-left[i].x, left[i].y};
    }

    float ear_left = compute_ear(left);
    float ear_right = compute_ear(right);
    EXPECT_NEAR(ear_left, ear_right, 1e-5F);
}

TEST(EARComputation, ear_precision_within_1e5) {
    // Hand-calculate EAR for known landmarks
    auto eye = make_eye(20.0F, 3.0F);
    // v1 = dist((-5,3), (-5,-3)) = 6.0
    // v2 = dist((5,3), (5,-3)) = 6.0
    // h = dist((-10,0), (10,0)) = 20.0
    // EAR = (6+6)/(2*20) = 0.3
    float ear = compute_ear(eye);
    EXPECT_NEAR(ear, 0.3F, 1e-5F);
}

TEST(EARComputation, degenerate_collinear_landmarks) {
    // All 6 points on the same horizontal line (y=0)
    std::array<Point2D, 6> eye = {{
        {0.0F, 0.0F}, {5.0F, 0.0F}, {10.0F, 0.0F},
        {15.0F, 0.0F}, {10.0F, 0.0F}, {5.0F, 0.0F}
    }};
    float ear = compute_ear(eye);
    // v1 = dist(p1,p5) = 0, v2 = dist(p2,p4) = 0 -> EAR = 0
    EXPECT_FLOAT_EQ(ear, 0.0F);
}

TEST(EARComputation, zero_horizontal_axis) {
    // p0 == p3 -> horizontal distance is zero
    std::array<Point2D, 6> eye = {{
        {5.0F, 5.0F},   // p0
        {3.0F, 8.0F},   // p1
        {7.0F, 8.0F},   // p2
        {5.0F, 5.0F},   // p3 == p0
        {7.0F, 2.0F},   // p4
        {3.0F, 2.0F},   // p5
    }};
    float ear = compute_ear(eye);
    // Should return 0.0 (graceful handling, no crash/NaN)
    EXPECT_FLOAT_EQ(ear, 0.0F);
    EXPECT_FALSE(std::isnan(ear));
}

TEST(EARComputation, ear_average_of_both_eyes) {
    EyeLandmarks landmarks;
    landmarks.left = make_eye(30.0F, 4.5F);   // EAR = 0.3
    landmarks.right = make_eye(30.0F, 1.5F);  // EAR = 0.1

    float avg = compute_ear_average(landmarks);
    EXPECT_NEAR(avg, 0.2F, 1e-5F);
}

TEST(EARComputation, negative_coordinates) {
    // Landmarks with negative coordinates
    std::array<Point2D, 6> eye = {{
        {-20.0F, -10.0F},   // p0
        {-15.0F, -7.0F},    // p1
        {-5.0F, -7.0F},     // p2
        {0.0F, -10.0F},     // p3
        {-5.0F, -13.0F},    // p4
        {-15.0F, -13.0F},   // p5
    }};
    float ear = compute_ear(eye);
    // v1 = dist((-15,-7), (-15,-13)) = 6
    // v2 = dist((-5,-7), (-5,-13)) = 6
    // h = dist((-20,-10), (0,-10)) = 20
    // EAR = 12/40 = 0.3
    EXPECT_NEAR(ear, 0.3F, 1e-5F);
}

TEST(EARComputation, large_coordinates) {
    // Same proportions at large scale
    std::array<Point2D, 6> eye = {{
        {10000.0F, 10000.0F},
        {10005.0F, 10003.0F},
        {10015.0F, 10003.0F},
        {10020.0F, 10000.0F},
        {10015.0F, 9997.0F},
        {10005.0F, 9997.0F},
    }};
    float ear = compute_ear(eye);
    // v1 = dist((10005,10003),(10005,9997)) = 6
    // v2 = dist((10015,10003),(10015,9997)) = 6
    // h = dist((10000,10000),(10020,10000)) = 20
    // EAR = 12/40 = 0.3
    EXPECT_NEAR(ear, 0.3F, 1e-4F);
}
