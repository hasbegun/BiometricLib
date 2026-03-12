#include <iris/nodes/fusion_extrapolator.hpp>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

#include <iris/core/node_registry.hpp>
#include <iris/utils/geometry_utils.hpp>

namespace iris {

FusionExtrapolator::FusionExtrapolator(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto get = [&](const std::string& key, double& out) {
        auto it = node_params.find(key);
        if (it != node_params.end()) {
            out = std::stod(it->second);
        }
    };

    // Nested params with dot-separated keys
    auto get_nested = [&](const std::string& prefix, const std::string& key,
                           double& out) {
        auto it = node_params.find(prefix + "." + key);
        if (it != node_params.end()) {
            out = std::stod(it->second);
        }
    };

    // YAML nests dphi under "params": circle_extrapolation.params.dphi
    get_nested("circle_extrapolation.params", "dphi",
               params_.circle_extrapolation.dphi);
    get_nested("ellipse_fit.params", "dphi", params_.ellipse_fit.dphi);
    get("algorithm_switch_std_threshold",
        params_.algorithm_switch_std_threshold);
    get("algorithm_switch_std_conditioned_multiplier",
        params_.algorithm_switch_std_conditioned_multiplier);
}

namespace {

/// Compute relative standard deviation (std/mean) of a vector.
double relative_std(const std::vector<double>& values) {
    if (values.size() < 2) return 0.0;

    const double sum = std::accumulate(values.begin(), values.end(), 0.0);
    const double mean = sum / static_cast<double>(values.size());
    if (std::abs(mean) < 1e-12) return 0.0;

    double sq_sum = 0.0;
    for (const auto& v : values) {
        const double diff = v - mean;
        sq_sum += diff * diff;
    }
    const double stddev =
        std::sqrt(sq_sum / static_cast<double>(values.size()));
    return stddev / std::abs(mean);
}

}  // namespace

Result<GeometryPolygons> FusionExtrapolator::run(
    const GeometryPolygons& polygons,
    const EyeCenters& eye_centers) const {
    // 1. Compute radius_std from INPUT polygons (before extrapolation)
    //    Matches Python: rhos_iris from input_polygons, not circle_poly
    auto [rhos_iris, phis_iris_unused] = cartesian2polar(
        polygons.iris, eye_centers.iris_center.x,
        eye_centers.iris_center.y);
    auto [rhos_pupil, phis_pupil_unused] = cartesian2polar(
        polygons.pupil, eye_centers.pupil_center.x,
        eye_centers.pupil_center.y);
    double radius_std_iris = relative_std(rhos_iris);
    double radius_std_pupil = relative_std(rhos_pupil);

    // 2. Run circle extrapolation
    LinearExtrapolator circle_extrap(params_.circle_extrapolation);
    auto circle_result = circle_extrap.run(polygons, eye_centers);
    if (!circle_result.has_value()) {
        return circle_result;
    }

    // 3. Run ellipse fitting
    EllipseFitter ellipse_fit(params_.ellipse_fit);
    auto ellipse_result = ellipse_fit.run(polygons);
    if (!ellipse_result.has_value()) {
        return make_error(ellipse_result.error().code,
                          ellipse_result.error().message);
    }

    // If ellipse fit returned nullopt (pupil not inside iris), use circle
    if (!ellipse_result.value().has_value()) {
        return circle_result;
    }

    auto& circle_poly = circle_result.value();
    auto& ellipse_poly = ellipse_result.value().value();

    // 4. Compute centroids of pupil arrays (matching Python: np.mean(pupil, axis=0))
    auto centroid = [](const Contour& c) -> cv::Point2d {
        double sx = 0.0;
        double sy = 0.0;
        for (const auto& p : c.points) {
            sx += static_cast<double>(p.x);
            sy += static_cast<double>(p.y);
        }
        const double n = static_cast<double>(c.points.size());
        return {sx / n, sy / n};
    };
    auto circle_center = centroid(circle_poly.pupil);
    auto ellipse_center = centroid(ellipse_poly.pupil);

    // 5. Compute element-wise squared radii ratios
    //    Matching Python: iris_sq / pupil_sq for corresponding points,
    //    both centered on the SAME center point
    auto squared_rel_radii = [](const Contour& iris, const Contour& pupil,
                                double cx, double cy) -> std::vector<double> {
        const size_t n = std::min(iris.points.size(), pupil.points.size());
        std::vector<double> ratios;
        ratios.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            const double ix = static_cast<double>(iris.points[i].x) - cx;
            const double iy = static_cast<double>(iris.points[i].y) - cy;
            const double px = static_cast<double>(pupil.points[i].x) - cx;
            const double py = static_cast<double>(pupil.points[i].y) - cy;
            const double iris_sq = ix * ix + iy * iy;
            const double pupil_sq = px * px + py * py;
            ratios.push_back(iris_sq / std::max(pupil_sq, 1e-12));
        }
        return ratios;
    };

    // cc: circle iris & circle pupil, both centered on circle pupil centroid
    auto cc_ratios = squared_rel_radii(
        circle_poly.iris, circle_poly.pupil,
        circle_center.x, circle_center.y);
    // ee: ellipse iris & ellipse pupil, both centered on ellipse pupil centroid
    auto ee_ratios = squared_rel_radii(
        ellipse_poly.iris, ellipse_poly.pupil,
        ellipse_center.x, ellipse_center.y);
    // ec: ellipse iris & circle pupil, both centered on circle pupil centroid
    auto ec_ratios = squared_rel_radii(
        ellipse_poly.iris, circle_poly.pupil,
        circle_center.x, circle_center.y);

    double cc_reg = relative_std(cc_ratios);
    double ee_reg = relative_std(ee_ratios);
    double ec_reg = relative_std(ec_ratios);

    // 6. Switch condition
    bool spread_ok = (radius_std_iris + radius_std_pupil) >=
                     params_.algorithm_switch_std_threshold;
    bool reg_ok = std::min(ee_reg, ec_reg) <=
                  (cc_reg + radius_std_iris *
                                params_.algorithm_switch_std_conditioned_multiplier);

    // 7. Decision with mixed result branch (matching Python)
    if (spread_ok && reg_ok) {
        if (ee_reg <= ec_reg) {
            // Full ellipse result
            GeometryPolygons result;
            result.iris = ellipse_poly.iris;
            result.pupil = ellipse_poly.pupil;
            result.eyeball = polygons.eyeball;
            return result;
        }
        // Mixed: ellipse iris + circle pupil
        GeometryPolygons result;
        result.iris = ellipse_poly.iris;
        result.pupil = circle_poly.pupil;
        result.eyeball = polygons.eyeball;
        return result;
    }

    return circle_result;
}

IRIS_REGISTER_NODE("iris.FusionExtrapolation", FusionExtrapolator);

}  // namespace iris
