#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include <eyetrack/core/error.hpp>
#include <eyetrack/core/types.hpp>

namespace eyetrack {

/// Input validation for incoming messages.
/// All methods return success or InvalidInput error.
struct InputValidator {
    /// Validate frame dimensions (1x1 to 4096x4096).
    [[nodiscard]] static Result<void> validate_frame_dimensions(
        uint32_t width, uint32_t height);

    /// Validate frame size (max 10MB).
    [[nodiscard]] static Result<void> validate_frame_size(size_t bytes);

    /// Validate timestamp freshness (within max_age of now).
    [[nodiscard]] static Result<void> validate_timestamp(
        std::chrono::steady_clock::time_point timestamp,
        std::chrono::seconds max_age = std::chrono::seconds(5));

    /// Validate client ID format (alphanumeric + underscore, 1-64 chars).
    [[nodiscard]] static Result<void> validate_client_id(
        const std::string& client_id);

    /// Validate calibration points (4-49 points, coordinates in [0,1]).
    [[nodiscard]] static Result<void> validate_calibration_points(
        const std::vector<Point2D>& points);
};

}  // namespace eyetrack
