#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace iris {

/// All error categories in the iris pipeline.
enum class ErrorCode : uint16_t {
    Ok = 0,
    SegmentationFailed,
    VectorizationFailed,
    GeometryEstimationFailed,
    NormalizationFailed,
    EncodingFailed,
    MatchingFailed,
    ValidationFailed,
    CryptoFailed,
    IoFailed,
    Cancelled,
    ConfigInvalid,
    ModelCorrupted,
    TemplateAggregationIncompatible,
    IdentityValidationFailed,
    KeyInvalid,
    KeyExpired,
    PipelineFailed,
};

/// Human-readable name for an error code.
constexpr std::string_view error_code_name(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::Ok:
            return "Ok";
        case ErrorCode::SegmentationFailed:
            return "SegmentationFailed";
        case ErrorCode::VectorizationFailed:
            return "VectorizationFailed";
        case ErrorCode::GeometryEstimationFailed:
            return "GeometryEstimationFailed";
        case ErrorCode::NormalizationFailed:
            return "NormalizationFailed";
        case ErrorCode::EncodingFailed:
            return "EncodingFailed";
        case ErrorCode::MatchingFailed:
            return "MatchingFailed";
        case ErrorCode::ValidationFailed:
            return "ValidationFailed";
        case ErrorCode::CryptoFailed:
            return "CryptoFailed";
        case ErrorCode::IoFailed:
            return "IoFailed";
        case ErrorCode::Cancelled:
            return "Cancelled";
        case ErrorCode::ConfigInvalid:
            return "ConfigInvalid";
        case ErrorCode::ModelCorrupted:
            return "ModelCorrupted";
        case ErrorCode::TemplateAggregationIncompatible:
            return "TemplateAggregationIncompatible";
        case ErrorCode::IdentityValidationFailed:
            return "IdentityValidationFailed";
        case ErrorCode::KeyInvalid:
            return "KeyInvalid";
        case ErrorCode::KeyExpired:
            return "KeyExpired";
        case ErrorCode::PipelineFailed:
            return "PipelineFailed";
    }
    return "Unknown";
}

/// Error details returned from pipeline operations.
struct IrisError {
    ErrorCode code;
    std::string message;
    std::string detail;
};

/// Result type used throughout the library.
template <typename T>
using Result = std::expected<T, IrisError>;

/// Convenience: create an unexpected IrisError.
inline std::unexpected<IrisError> make_error(
    ErrorCode code,
    std::string message,
    std::string detail = {}) {
    return std::unexpected<IrisError>({code, std::move(message), std::move(detail)});
}

}  // namespace iris
