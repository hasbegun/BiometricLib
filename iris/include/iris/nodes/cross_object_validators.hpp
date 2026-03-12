#pragma once

#include <string_view>
#include <unordered_map>
#include <variant>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

// ---------------------------------------------------------------------------
// EyeCentersInsideImageValidator
// ---------------------------------------------------------------------------

class EyeCentersInsideImageValidator
    : public Algorithm<EyeCentersInsideImageValidator> {
public:
    static constexpr std::string_view kName =
        "EyeCentersInsideImageValidator";

    struct Params {
        double min_distance_to_border = 0.0;
    };

    EyeCentersInsideImageValidator() = default;
    explicit EyeCentersInsideImageValidator(const Params& params)
        : params_(params) {}
    explicit EyeCentersInsideImageValidator(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<std::monostate> run(const IRImage& ir_image,
                                const EyeCenters& eye_centers) const;

private:
    Params params_;
};

// ---------------------------------------------------------------------------
// ExtrapolatedPolygonsInsideImageValidator
// ---------------------------------------------------------------------------

class ExtrapolatedPolygonsInsideImageValidator
    : public Algorithm<ExtrapolatedPolygonsInsideImageValidator> {
public:
    static constexpr std::string_view kName =
        "ExtrapolatedPolygonsInsideImageValidator";

    struct Params {
        double min_pupil_allowed_percentage = 0.0;
        double min_iris_allowed_percentage = 0.0;
        double min_eyeball_allowed_percentage = 0.0;
    };

    ExtrapolatedPolygonsInsideImageValidator() = default;
    explicit ExtrapolatedPolygonsInsideImageValidator(const Params& params)
        : params_(params) {}
    explicit ExtrapolatedPolygonsInsideImageValidator(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<std::monostate> run(
        const IRImage& ir_image,
        const GeometryPolygons& geometry_polygons) const;

private:
    Params params_;
};

}  // namespace iris
