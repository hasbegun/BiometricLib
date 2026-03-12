#pragma once

#include <array>
#include <vector>

#include <eyetrack/core/types.hpp>

namespace eyetrack {

/// Euclidean distance between two 2D points.
[[nodiscard]] float euclidean_distance(Point2D a, Point2D b) noexcept;

/// Compute the Eye Aspect Ratio (EAR) for a single eye.
///
/// EAR = (|p1-p5| + |p2-p4|) / (2 * |p0-p3|)
///
/// 6-point landmark layout:
///   p1  p2
/// p0      p3
///   p5  p4
///
/// Returns 0.0 if horizontal distance is zero (degenerate case).
[[nodiscard]] float compute_ear(const std::array<Point2D, 6>& eye) noexcept;

/// Average EAR across both eyes.
[[nodiscard]] float compute_ear_average(const EyeLandmarks& landmarks) noexcept;

/// Normalize a point to [0,1] range given frame dimensions.
/// Returns (x/width, y/height).
[[nodiscard]] Point2D normalize_point(Point2D p, float width, float height) noexcept;

/// Build 2nd-degree polynomial feature vector: [1, x, y, x^2, y^2, xy].
[[nodiscard]] std::vector<float> polynomial_features(float x, float y);

}  // namespace eyetrack
