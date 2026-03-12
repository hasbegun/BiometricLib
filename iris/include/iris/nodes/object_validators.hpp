#pragma once

#include <string_view>
#include <unordered_map>
#include <variant>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

// ---------------------------------------------------------------------------
// Pupil2IrisPropertyValidator
// ---------------------------------------------------------------------------

class Pupil2IrisPropertyValidator
    : public Algorithm<Pupil2IrisPropertyValidator> {
public:
    static constexpr std::string_view kName = "Pupil2IrisPropertyValidator";

    struct Params {
        double min_allowed_diameter_ratio = 0.0001;
        double max_allowed_diameter_ratio = 0.9999;
        double max_allowed_center_dist_ratio = 0.9999;
    };

    Pupil2IrisPropertyValidator() = default;
    explicit Pupil2IrisPropertyValidator(const Params& params)
        : params_(params) {}
    explicit Pupil2IrisPropertyValidator(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<std::monostate> run(
        const PupilToIrisProperty& property) const;

private:
    Params params_;
};

// ---------------------------------------------------------------------------
// OffgazeValidator
// ---------------------------------------------------------------------------

class OffgazeValidator : public Algorithm<OffgazeValidator> {
public:
    static constexpr std::string_view kName = "OffgazeValidator";

    struct Params {
        double max_allowed_offgaze = 1.0;
    };

    OffgazeValidator() = default;
    explicit OffgazeValidator(const Params& params) : params_(params) {}
    explicit OffgazeValidator(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<std::monostate> run(const Offgaze& offgaze) const;

private:
    Params params_;
};

// ---------------------------------------------------------------------------
// OcclusionValidator
// ---------------------------------------------------------------------------

class OcclusionValidator : public Algorithm<OcclusionValidator> {
public:
    static constexpr std::string_view kName = "OcclusionValidator";

    struct Params {
        double min_allowed_occlusion = 0.0;
    };

    OcclusionValidator() = default;
    explicit OcclusionValidator(const Params& params) : params_(params) {}
    explicit OcclusionValidator(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<std::monostate> run(const EyeOcclusion& occlusion) const;

private:
    Params params_;
};

// ---------------------------------------------------------------------------
// IsPupilInsideIrisValidator
// ---------------------------------------------------------------------------

class IsPupilInsideIrisValidator
    : public Algorithm<IsPupilInsideIrisValidator> {
public:
    static constexpr std::string_view kName = "IsPupilInsideIrisValidator";

    IsPupilInsideIrisValidator() = default;
    explicit IsPupilInsideIrisValidator(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<std::monostate> run(
        const GeometryPolygons& geometry_polygons) const;
};

// ---------------------------------------------------------------------------
// SharpnessValidator
// ---------------------------------------------------------------------------

class SharpnessValidator : public Algorithm<SharpnessValidator> {
public:
    static constexpr std::string_view kName = "SharpnessValidator";

    struct Params {
        double min_sharpness = 0.0;
    };

    SharpnessValidator() = default;
    explicit SharpnessValidator(const Params& params) : params_(params) {}
    explicit SharpnessValidator(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<std::monostate> run(const Sharpness& sharpness) const;

private:
    Params params_;
};

// ---------------------------------------------------------------------------
// IsMaskTooSmallValidator
// ---------------------------------------------------------------------------

class IsMaskTooSmallValidator : public Algorithm<IsMaskTooSmallValidator> {
public:
    static constexpr std::string_view kName = "IsMaskTooSmallValidator";

    struct Params {
        int min_maskcodes_size = 0;
    };

    IsMaskTooSmallValidator() = default;
    explicit IsMaskTooSmallValidator(const Params& params)
        : params_(params) {}
    explicit IsMaskTooSmallValidator(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<std::monostate> run(const IrisTemplate& iris_template) const;

private:
    Params params_;
};

// ---------------------------------------------------------------------------
// PolygonsLengthValidator
// ---------------------------------------------------------------------------

class PolygonsLengthValidator : public Algorithm<PolygonsLengthValidator> {
public:
    static constexpr std::string_view kName = "PolygonsLengthValidator";

    struct Params {
        double min_iris_length = 150.0;
        double min_pupil_length = 75.0;
    };

    PolygonsLengthValidator() = default;
    explicit PolygonsLengthValidator(const Params& params)
        : params_(params) {}
    explicit PolygonsLengthValidator(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<std::monostate> run(
        const GeometryPolygons& geometry_polygons) const;

private:
    Params params_;
};

}  // namespace iris
