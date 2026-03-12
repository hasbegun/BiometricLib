#pragma once

#include <string_view>
#include <unordered_map>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Extrapolates geometry contours via linear interpolation in polar space.
///
/// Converts contours to polar coordinates, pads with 3 copies for
/// periodicity, interpolates rho at uniform dphi intervals, and
/// converts back to Cartesian.
class LinearExtrapolator : public Algorithm<LinearExtrapolator> {
public:
    static constexpr std::string_view kName = "LinearExtrapolator";

    struct Params {
        double dphi = 0.9;  // Angular sampling step (degrees)
    };

    LinearExtrapolator() = default;
    explicit LinearExtrapolator(const Params& params) : params_(params) {}
    explicit LinearExtrapolator(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<GeometryPolygons> run(const GeometryPolygons& polygons,
                                  const EyeCenters& eye_centers) const;

private:
    Params params_;
};

}  // namespace iris
