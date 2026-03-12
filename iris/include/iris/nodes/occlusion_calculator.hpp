#pragma once

#include <string_view>
#include <unordered_map>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Compute the fraction of visible iris, accounting for noise masks,
/// eyelid occlusion, and quantile angular filtering.
class OcclusionCalculator : public Algorithm<OcclusionCalculator> {
public:
    static constexpr std::string_view kName = "OcclusionCalculator";

    struct Params {
        /// Angular quantile in degrees [0, 90]. 90 = full iris, 30 = center third.
        double quantile_angle = 90.0;
    };

    OcclusionCalculator() = default;
    explicit OcclusionCalculator(const Params& params) : params_(params) {}
    explicit OcclusionCalculator(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<EyeOcclusion> run(const GeometryPolygons& extrapolated_polygons,
                              const NoiseMask& noise_mask,
                              const EyeOrientation& eye_orientation,
                              const EyeCenters& eye_centers) const;

private:
    Params params_;

    Contour get_quantile_points(const Contour& iris_coords,
                                const EyeOrientation& eye_orientation) const;
};

}  // namespace iris
