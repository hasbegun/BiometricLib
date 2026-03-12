#pragma once

#include <vector>

#include <Eigen/Dense>

#include <eyetrack/core/error.hpp>
#include <eyetrack/core/types.hpp>

namespace eyetrack {

/// Eigen-dependent calibration data.
/// Deliberately separated from core CalibrationProfile to confine Eigen dependency.
struct CalibrationTransform {
    Eigen::MatrixXf transform_left;   // polynomial coefficients for left eye
    Eigen::MatrixXf transform_right;  // polynomial coefficients for right eye
};

/// Fit polynomial coefficients mapping pupil positions to screen coordinates.
///
/// Uses least-squares (Ax = b) with 2nd-degree polynomial features [1, x, y, x², y², xy].
/// Requires at least 3 (pupil, screen) point pairs.
///
/// @param pupil_positions  Pupil center positions (input features)
/// @param screen_points    Corresponding screen coordinates (targets)
/// @return Coefficient matrix (6 x 2) mapping features to (screen_x, screen_y),
///         or CalibrationFailed on error.
[[nodiscard]] Result<Eigen::MatrixXf> least_squares_fit(
    const std::vector<Point2D>& pupil_positions,
    const std::vector<Point2D>& screen_points);

/// Apply polynomial mapping to a pupil position using fitted coefficients.
///
/// @param pupil   Pupil center position
/// @param coeffs  Coefficient matrix (6 x 2) from least_squares_fit
/// @return Mapped screen coordinate
[[nodiscard]] Point2D apply_polynomial_mapping(
    Point2D pupil,
    const Eigen::MatrixXf& coeffs);

}  // namespace eyetrack
