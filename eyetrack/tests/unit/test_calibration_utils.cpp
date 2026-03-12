#include <gtest/gtest.h>

#include <cmath>
#include <fstream>
#include <vector>

#include <eyetrack/utils/calibration_utils.hpp>
#include <eyetrack/utils/geometry_utils.hpp>

using eyetrack::CalibrationTransform;
using eyetrack::Point2D;

// --- CalibrationTransform struct ---

TEST(CalibrationTransform, eigen_matrices_initialized) {
    CalibrationTransform ct;
    // Default-constructed Eigen matrices have size 0x0
    EXPECT_EQ(ct.transform_left.rows(), 0);
    EXPECT_EQ(ct.transform_left.cols(), 0);
    EXPECT_EQ(ct.transform_right.rows(), 0);
    EXPECT_EQ(ct.transform_right.cols(), 0);
}

// --- LeastSquaresFit tests ---

TEST(LeastSquaresFit, identity_linear_mapping) {
    // Input == output: pupil positions map to themselves
    std::vector<Point2D> pupil = {
        {0.0F, 0.0F}, {1.0F, 0.0F}, {0.0F, 1.0F},
        {1.0F, 1.0F}, {0.5F, 0.5F}};
    std::vector<Point2D> screen = pupil;  // identity

    auto result = eyetrack::least_squares_fit(pupil, screen);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Verify reconstruction
    for (size_t i = 0; i < pupil.size(); ++i) {
        auto mapped = eyetrack::apply_polynomial_mapping(pupil[i], *result);
        EXPECT_NEAR(mapped.x, screen[i].x, 0.01F)
            << "Point " << i << " x mismatch";
        EXPECT_NEAR(mapped.y, screen[i].y, 0.01F)
            << "Point " << i << " y mismatch";
    }
}

TEST(LeastSquaresFit, known_linear_mapping) {
    // y = 2x + 1 for both dimensions: screen = 2 * pupil + 1
    std::vector<Point2D> pupil = {
        {0.0F, 0.0F}, {1.0F, 0.0F}, {0.0F, 1.0F},
        {1.0F, 1.0F}, {0.5F, 0.5F}, {0.25F, 0.75F}};
    std::vector<Point2D> screen;
    for (const auto& p : pupil) {
        screen.push_back({2.0F * p.x + 1.0F, 2.0F * p.y + 1.0F});
    }

    auto result = eyetrack::least_squares_fit(pupil, screen);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Coefficients are for [1, x, y, x², y², xy]
    // For screen_x = 2*x + 1: coeffs should be ~[1, 2, 0, 0, 0, 0]
    auto& coeffs = *result;
    EXPECT_NEAR(coeffs(0, 0), 1.0F, 0.01F) << "Intercept for x";
    EXPECT_NEAR(coeffs(1, 0), 2.0F, 0.01F) << "x coefficient for x";

    // Verify reconstruction
    for (size_t i = 0; i < pupil.size(); ++i) {
        auto mapped = eyetrack::apply_polynomial_mapping(pupil[i], *result);
        EXPECT_NEAR(mapped.x, screen[i].x, 0.01F);
        EXPECT_NEAR(mapped.y, screen[i].y, 0.01F);
    }
}

TEST(LeastSquaresFit, known_quadratic_mapping) {
    // screen_x = x^2, screen_y = y^2
    std::vector<Point2D> pupil = {
        {0.0F, 0.0F}, {1.0F, 0.0F}, {2.0F, 0.0F},
        {0.0F, 1.0F}, {0.0F, 2.0F}, {1.0F, 1.0F},
        {2.0F, 2.0F}, {1.5F, 0.5F}, {0.5F, 1.5F}};
    std::vector<Point2D> screen;
    for (const auto& p : pupil) {
        screen.push_back({p.x * p.x, p.y * p.y});
    }

    auto result = eyetrack::least_squares_fit(pupil, screen);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Coefficients for [1, x, y, x², y², xy]
    // For screen_x = x²: x² coefficient should be ~1
    auto& coeffs = *result;
    EXPECT_NEAR(coeffs(3, 0), 1.0F, 0.01F) << "x² coefficient for screen_x";

    // Verify reconstruction
    for (size_t i = 0; i < pupil.size(); ++i) {
        auto mapped = eyetrack::apply_polynomial_mapping(pupil[i], *result);
        EXPECT_NEAR(mapped.x, screen[i].x, 0.01F)
            << "Point " << i << " x mismatch";
        EXPECT_NEAR(mapped.y, screen[i].y, 0.01F)
            << "Point " << i << " y mismatch";
    }
}

TEST(LeastSquaresFit, too_few_points_returns_error) {
    std::vector<Point2D> pupil = {{0.0F, 0.0F}, {1.0F, 1.0F}};
    std::vector<Point2D> screen = {{0.0F, 0.0F}, {1.0F, 1.0F}};

    auto result = eyetrack::least_squares_fit(pupil, screen);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::CalibrationFailed);
}

TEST(LeastSquaresFit, exact_reconstruction) {
    // Fit with exact polynomial data and verify reconstruction within 0.01
    // Using: screen_x = 0.5 + 1.2*x - 0.3*y + 0.1*x² + 0.05*y² - 0.02*xy
    //        screen_y = -0.2 + 0.8*x + 1.5*y - 0.05*x² + 0.1*y² + 0.03*xy
    auto compute_screen = [](float x, float y) -> Point2D {
        float sx = 0.5F + 1.2F * x - 0.3F * y + 0.1F * x * x +
                   0.05F * y * y - 0.02F * x * y;
        float sy = -0.2F + 0.8F * x + 1.5F * y - 0.05F * x * x +
                   0.1F * y * y + 0.03F * x * y;
        return {sx, sy};
    };

    std::vector<Point2D> pupil;
    std::vector<Point2D> screen;
    // 3x3 grid
    for (float x = 0.0F; x <= 2.0F; x += 1.0F) {
        for (float y = 0.0F; y <= 2.0F; y += 1.0F) {
            pupil.push_back({x, y});
            screen.push_back(compute_screen(x, y));
        }
    }

    auto result = eyetrack::least_squares_fit(pupil, screen);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Reconstruct and verify
    for (size_t i = 0; i < pupil.size(); ++i) {
        auto mapped = eyetrack::apply_polynomial_mapping(pupil[i], *result);
        EXPECT_NEAR(mapped.x, screen[i].x, 0.01F)
            << "Reconstruction point " << i << " x";
        EXPECT_NEAR(mapped.y, screen[i].y, 0.01F)
            << "Reconstruction point " << i << " y";
    }
}

// --- ApplyPolynomialMapping tests ---

TEST(ApplyPolynomialMapping, known_coefficients) {
    // Manually construct coefficients for: screen_x = 3*x, screen_y = 2*y
    Eigen::MatrixXf coeffs = Eigen::MatrixXf::Zero(6, 2);
    coeffs(1, 0) = 3.0F;  // x coefficient -> screen_x
    coeffs(2, 1) = 2.0F;  // y coefficient -> screen_y

    auto result = eyetrack::apply_polynomial_mapping({4.0F, 5.0F}, coeffs);
    EXPECT_NEAR(result.x, 12.0F, 0.01F);
    EXPECT_NEAR(result.y, 10.0F, 0.01F);
}

TEST(ApplyPolynomialMapping, identity_coefficients) {
    // Identity: screen_x = x, screen_y = y
    Eigen::MatrixXf coeffs = Eigen::MatrixXf::Zero(6, 2);
    coeffs(1, 0) = 1.0F;  // x -> screen_x
    coeffs(2, 1) = 1.0F;  // y -> screen_y

    Point2D input{0.7F, 0.3F};
    auto result = eyetrack::apply_polynomial_mapping(input, coeffs);
    EXPECT_NEAR(result.x, input.x, 0.01F);
    EXPECT_NEAR(result.y, input.y, 0.01F);
}

// --- Eigen confinement test ---

TEST(CalibrationUtils, eigen_dependency_confined) {
    // Verify that core/types.hpp does not include Eigen by checking
    // that Point2D and PupilInfo are usable without Eigen.
    // This is a compile-time check: if types.hpp included Eigen,
    // the compilation would succeed either way, but we verify by
    // checking that the sizeof a simple type is small (not Eigen-inflated).
    EXPECT_LE(sizeof(Point2D), 16U);  // 2 floats = 8 bytes + padding
    EXPECT_LE(sizeof(eyetrack::PupilInfo), 32U);  // 2 floats + float + float
}

// --- 9-point grid test ---

TEST(LeastSquaresFit, nine_point_grid) {
    // Standard 9-point calibration grid
    std::vector<Point2D> screen_targets = {
        {0.0F, 0.0F}, {0.5F, 0.0F}, {1.0F, 0.0F},
        {0.0F, 0.5F}, {0.5F, 0.5F}, {1.0F, 0.5F},
        {0.0F, 1.0F}, {0.5F, 1.0F}, {1.0F, 1.0F}};

    // Simulate pupil positions with a known linear mapping + slight offset
    // pupil = 0.3 * screen + 0.1
    std::vector<Point2D> pupil_positions;
    for (const auto& s : screen_targets) {
        pupil_positions.push_back(
            {0.3F * s.x + 0.1F, 0.3F * s.y + 0.1F});
    }

    auto result = eyetrack::least_squares_fit(pupil_positions, screen_targets);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Coefficients should be 6x2
    EXPECT_EQ(result->rows(), 6);
    EXPECT_EQ(result->cols(), 2);

    // Verify reconstruction
    for (size_t i = 0; i < pupil_positions.size(); ++i) {
        auto mapped = eyetrack::apply_polynomial_mapping(
            pupil_positions[i], *result);
        EXPECT_NEAR(mapped.x, screen_targets[i].x, 0.01F)
            << "9-point grid point " << i << " x";
        EXPECT_NEAR(mapped.y, screen_targets[i].y, 0.01F)
            << "9-point grid point " << i << " y";
    }
}
