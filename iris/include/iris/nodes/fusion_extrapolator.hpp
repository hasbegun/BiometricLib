#pragma once

#include <string_view>
#include <unordered_map>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>
#include <iris/nodes/ellipse_fitter.hpp>
#include <iris/nodes/linear_extrapolator.hpp>

namespace iris {

/// Fuses circle-based and ellipse-based geometry extrapolation.
///
/// Runs both LinearExtrapolator and EllipseFitter, then selects the
/// best result based on shape regularity statistics.
class FusionExtrapolator : public Algorithm<FusionExtrapolator> {
public:
    static constexpr std::string_view kName = "FusionExtrapolator";

    struct Params {
        LinearExtrapolator::Params circle_extrapolation;
        EllipseFitter::Params ellipse_fit;
        double algorithm_switch_std_threshold = 0.014;
        double algorithm_switch_std_conditioned_multiplier = 2.0;
    };

    FusionExtrapolator() = default;
    explicit FusionExtrapolator(const Params& params) : params_(params) {}
    explicit FusionExtrapolator(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<GeometryPolygons> run(const GeometryPolygons& polygons,
                                  const EyeCenters& eye_centers) const;

private:
    Params params_;
};

}  // namespace iris
