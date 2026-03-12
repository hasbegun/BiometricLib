#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Determines an offgaze score by assembling the eccentricity of iris and pupil
/// polygons. A perfect circle has eccentricity 0; a line has eccentricity 1.
class EccentricityOffgazeEstimation
    : public Algorithm<EccentricityOffgazeEstimation> {
public:
    static constexpr std::string_view kName = "EccentricityOffgazeEstimation";

    struct Params {
        /// "moments", "ellipse_fit"
        std::string eccentricity_method = "moments";
        /// "min", "max", "mean", "only_pupil", "only_iris"
        std::string assembling_method = "min";
    };

    EccentricityOffgazeEstimation() = default;
    explicit EccentricityOffgazeEstimation(const Params& params)
        : params_(params) {}
    explicit EccentricityOffgazeEstimation(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<Offgaze> run(const GeometryPolygons& geometries) const;

private:
    Params params_;

    static double eccentricity_via_moments(const Contour& shape);
    static double eccentricity_via_ellipse_fit(const Contour& shape);
};

}  // namespace iris
