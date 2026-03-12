#pragma once

#include <string_view>
#include <unordered_map>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Interpolate contour points so that no two adjacent points are further
/// apart than a fraction of the iris diameter.
class ContourInterpolator : public Algorithm<ContourInterpolator> {
public:
    static constexpr std::string_view kName = "ContourInterpolator";

    struct Params {
        double max_distance_between_boundary_points = 0.01;
    };

    ContourInterpolator() = default;
    explicit ContourInterpolator(const Params& params) : params_(params) {}
    explicit ContourInterpolator(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<GeometryPolygons> run(const GeometryPolygons& polygons) const;

private:
    Params params_;
};

}  // namespace iris
