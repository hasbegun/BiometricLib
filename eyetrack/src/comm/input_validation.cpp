#include <eyetrack/comm/input_validation.hpp>

#include <algorithm>
#include <cctype>

namespace eyetrack {

Result<void> InputValidator::validate_frame_dimensions(uint32_t width,
                                                        uint32_t height) {
    if (width == 0 || height == 0) {
        return make_error(ErrorCode::InvalidInput,
                          "Frame dimensions must be at least 1x1");
    }
    if (width > 4096 || height > 4096) {
        return make_error(ErrorCode::InvalidInput,
                          "Frame dimensions must not exceed 4096x4096");
    }
    return {};
}

Result<void> InputValidator::validate_frame_size(size_t bytes) {
    constexpr size_t kMaxFrameSize = 10ULL * 1024 * 1024;  // 10MB
    if (bytes > kMaxFrameSize) {
        return make_error(ErrorCode::InvalidInput,
                          "Frame size exceeds 10MB limit");
    }
    return {};
}

Result<void> InputValidator::validate_timestamp(
    std::chrono::steady_clock::time_point timestamp,
    std::chrono::seconds max_age) {
    auto now = std::chrono::steady_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::seconds>(now - timestamp);
    if (age > max_age) {
        return make_error(ErrorCode::InvalidInput,
                          "Timestamp too old (>" +
                              std::to_string(max_age.count()) + "s)");
    }
    return {};
}

Result<void> InputValidator::validate_client_id(const std::string& client_id) {
    if (client_id.empty()) {
        return make_error(ErrorCode::InvalidInput, "Client ID must not be empty");
    }
    if (client_id.size() > 64) {
        return make_error(ErrorCode::InvalidInput,
                          "Client ID must not exceed 64 characters");
    }
    auto valid = std::ranges::all_of(client_id, [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
    });
    if (!valid) {
        return make_error(ErrorCode::InvalidInput,
                          "Client ID must contain only alphanumeric characters "
                          "and underscores");
    }
    return {};
}

Result<void> InputValidator::validate_calibration_points(
    const std::vector<Point2D>& points) {
    if (points.size() < 4) {
        return make_error(ErrorCode::InvalidInput,
                          "At least 4 calibration points required");
    }
    if (points.size() > 49) {
        return make_error(ErrorCode::InvalidInput,
                          "At most 49 calibration points allowed");
    }
    for (const auto& p : points) {
        if (p.x < 0.0F || p.x > 1.0F || p.y < 0.0F || p.y > 1.0F) {
            return make_error(ErrorCode::InvalidInput,
                              "Calibration point coordinates must be in [0, 1]");
        }
    }
    return {};
}

}  // namespace eyetrack
