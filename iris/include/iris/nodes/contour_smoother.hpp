#pragma once

#include <string_view>
#include <unordered_map>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Smooths contour boundaries using polar-coordinate rolling median filtering.
///
/// Converts contours to polar coordinates around the eye center, detects
/// gaps, splits into arcs, interpolates at uniform angular intervals,
/// applies rolling median, and converts back.
class ContourSmoother : public Algorithm<ContourSmoother> {
public:
    static constexpr std::string_view kName = "ContourSmoother";

    struct Params {
        double dphi = 1.0;           // Angular sampling step (degrees)
        double kernel_size = 10.0;   // Rolling median kernel size (degrees)
        double gap_threshold = 10.0; // Gap detection threshold (degrees)
    };

    ContourSmoother() = default;
    explicit ContourSmoother(const Params& params) : params_(params) {}
    explicit ContourSmoother(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<GeometryPolygons> run(const GeometryPolygons& polygons,
                                 const EyeCenters& eye_centers) const;

private:
    Params params_;
};

}  // namespace iris
