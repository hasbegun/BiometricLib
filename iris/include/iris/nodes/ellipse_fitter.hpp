#pragma once

#include <optional>
#include <string_view>
#include <unordered_map>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Fits ellipses to iris and pupil contours using least-squares fitting.
///
/// Uses cv::fitEllipse, validates pupil-inside-iris, generates parametric
/// ellipse points, and refines pupil positions with original input points.
class EllipseFitter : public Algorithm<EllipseFitter> {
public:
    static constexpr std::string_view kName = "EllipseFitter";

    struct Params {
        double dphi = 1.0;  // Angular step (degrees) for parametric points
    };

    EllipseFitter() = default;
    explicit EllipseFitter(const Params& params) : params_(params) {}
    explicit EllipseFitter(
        const std::unordered_map<std::string, std::string>& node_params);

    /// Returns std::nullopt if pupil is not inside iris (validation fails).
    Result<std::optional<GeometryPolygons>> run(
        const GeometryPolygons& polygons) const;

private:
    Params params_;
};

}  // namespace iris
