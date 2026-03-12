#pragma once

#include <string_view>
#include <unordered_map>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Filter contour points that are too close to noise or eyeball boundary.
///
/// Creates a "forbidden zone" by dilating the combined noise + eyeball mask,
/// then removes iris and pupil contour points that fall within this zone.
class EyeballDistanceFilter : public Algorithm<EyeballDistanceFilter> {
public:
    static constexpr std::string_view kName = "EyeballDistanceFilter";

    struct Params {
        double min_distance_to_noise_and_eyeball = 0.005;
    };

    EyeballDistanceFilter() = default;
    explicit EyeballDistanceFilter(const Params& params) : params_(params) {}
    explicit EyeballDistanceFilter(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<GeometryPolygons> run(const GeometryPolygons& polygons,
                                 const NoiseMask& noise_mask) const;

private:
    Params params_;
};

}  // namespace iris
