#pragma once

#include <cstdint>
#include <expected>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

namespace eyetrack {

enum class ErrorCode : uint16_t {
    Success = 0,
    FaceNotDetected,
    EyeNotDetected,
    PupilNotDetected,
    CalibrationFailed,
    CalibrationNotLoaded,
    ModelLoadFailed,
    ConfigInvalid,
    ConnectionFailed,
    Timeout,
    Cancelled,
    Unauthenticated,
    PermissionDenied,
    InvalidInput,
    RateLimited,
};

constexpr std::string_view error_code_name(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::Success:              return "Success";
        case ErrorCode::FaceNotDetected:      return "FaceNotDetected";
        case ErrorCode::EyeNotDetected:       return "EyeNotDetected";
        case ErrorCode::PupilNotDetected:     return "PupilNotDetected";
        case ErrorCode::CalibrationFailed:    return "CalibrationFailed";
        case ErrorCode::CalibrationNotLoaded: return "CalibrationNotLoaded";
        case ErrorCode::ModelLoadFailed:      return "ModelLoadFailed";
        case ErrorCode::ConfigInvalid:        return "ConfigInvalid";
        case ErrorCode::ConnectionFailed:     return "ConnectionFailed";
        case ErrorCode::Timeout:              return "Timeout";
        case ErrorCode::Cancelled:            return "Cancelled";
        case ErrorCode::Unauthenticated:      return "Unauthenticated";
        case ErrorCode::PermissionDenied:     return "PermissionDenied";
        case ErrorCode::InvalidInput:         return "InvalidInput";
        case ErrorCode::RateLimited:          return "RateLimited";
    }
    return "Unknown";
}

struct EyetrackError {
    ErrorCode code = ErrorCode::Success;
    std::string message;
    std::string detail;
};

std::ostream& operator<<(std::ostream& os, const EyetrackError& err);

template <typename T>
using Result = std::expected<T, EyetrackError>;

inline std::unexpected<EyetrackError> make_error(
    ErrorCode code,
    std::string message,
    std::string detail = {}) {
    return std::unexpected<EyetrackError>(
        {code, std::move(message), std::move(detail)});
}

}  // namespace eyetrack
