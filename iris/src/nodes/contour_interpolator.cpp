#include <iris/nodes/contour_interpolator.hpp>

#include <cmath>

#include <iris/core/node_registry.hpp>
#include <iris/utils/geometry_utils.hpp>

namespace iris {

ContourInterpolator::ContourInterpolator(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("max_distance_between_boundary_points");
    if (it != node_params.end()) {
        params_.max_distance_between_boundary_points = std::stod(it->second);
    }
}

namespace {

/// Interpolate a single contour so adjacent points are at most max_dist_px apart.
Contour interpolate_contour(const Contour& contour, double max_dist_px) {
    const auto& pts = contour.points;
    if (pts.size() < 2 || max_dist_px <= 0.0) {
        return contour;
    }

    Contour result;
    result.points.reserve(pts.size() * 2);  // rough estimate

    for (size_t i = 0; i < pts.size(); ++i) {
        const auto& current = pts[i];
        // Previous point with wraparound
        const auto& prev = pts[(i + pts.size() - 1) % pts.size()];

        const double dx = static_cast<double>(current.x - prev.x);
        const double dy = static_cast<double>(current.y - prev.y);
        const double dist = std::hypot(dx, dy);

        if (dist > max_dist_px) {
            // Insert interpolated points (excluding current — added below)
            const int num_points = static_cast<int>(std::ceil(dist / max_dist_px));
            for (int j = 1; j < num_points; ++j) {
                const double t = static_cast<double>(j) / static_cast<double>(num_points);
                const float interp_x = prev.x + static_cast<float>(t * dx);
                const float interp_y = prev.y + static_cast<float>(t * dy);
                result.points.emplace_back(interp_x, interp_y);
            }
        }

        result.points.push_back(current);
    }

    return result;
}

}  // namespace

Result<GeometryPolygons> ContourInterpolator::run(const GeometryPolygons& polygons) const {
    if (polygons.iris.points.empty()) {
        return make_error(ErrorCode::VectorizationFailed,
                          "Empty iris contour — cannot interpolate");
    }

    const double iris_diam = estimate_diameter(polygons.iris);
    const double max_dist_px = params_.max_distance_between_boundary_points * iris_diam;

    GeometryPolygons result;
    result.pupil = interpolate_contour(polygons.pupil, max_dist_px);
    result.iris = interpolate_contour(polygons.iris, max_dist_px);
    result.eyeball = interpolate_contour(polygons.eyeball, max_dist_px);

    return result;
}

IRIS_REGISTER_NODE("iris.ContourInterpolation", ContourInterpolator);

}  // namespace iris
