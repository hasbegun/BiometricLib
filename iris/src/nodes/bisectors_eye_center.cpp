#include <iris/nodes/bisectors_eye_center.hpp>

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

#include <iris/core/node_registry.hpp>
#include <iris/utils/geometry_utils.hpp>

namespace iris {

BisectorsEyeCenter::BisectorsEyeCenter(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto get_int = [&](const std::string& key, int& out) {
        auto it = node_params.find(key);
        if (it != node_params.end()) {
            out = std::stoi(it->second);
        }
    };
    auto get_dbl = [&](const std::string& key, double& out) {
        auto it = node_params.find(key);
        if (it != node_params.end()) {
            out = std::stod(it->second);
        }
    };
    get_int("num_bisectors", params_.num_bisectors);
    get_dbl("min_distance_between_sector_points",
            params_.min_distance_between_sector_points);
    get_int("max_iterations", params_.max_iterations);
}

namespace {

/// Sample perpendicular bisectors from random point pairs.
/// Returns (first_points, second_points) on each bisector line.
/// Each row is a 2D point defining the bisector line.
struct BisectorLines {
    std::vector<double> fst_x, fst_y;  // First point on each bisector
    std::vector<double> snd_x, snd_y;  // Second point on each bisector
};

BisectorLines calculate_perpendicular_bisectors(
    const Contour& polygon, int num_bisectors,
    double min_distance_px, int max_iterations) {
    const auto n = polygon.points.size();
    if (n < 2) {
        return {};
    }

    // Fixed seed for reproducibility (matches Python: 142857)
    std::mt19937 rng(142857);
    std::uniform_int_distribution<size_t> dist(0, n - 1);

    BisectorLines result;
    result.fst_x.reserve(static_cast<size_t>(num_bisectors));
    result.fst_y.reserve(static_cast<size_t>(num_bisectors));
    result.snd_x.reserve(static_cast<size_t>(num_bisectors));
    result.snd_y.reserve(static_cast<size_t>(num_bisectors));

    for (int iter = 0;
         iter < max_iterations &&
         static_cast<int>(result.fst_x.size()) < num_bisectors;
         ++iter) {
        for (int k = 0; k < num_bisectors; ++k) {
            const size_t i1 = dist(rng);
            const size_t i2 = dist(rng);
            if (i1 == i2) continue;

            const auto& p1 = polygon.points[i1];
            const auto& p2 = polygon.points[i2];

            const double dx = static_cast<double>(p2.x - p1.x);
            const double dy = static_cast<double>(p2.y - p1.y);
            const double d = std::hypot(dx, dy);

            if (d < min_distance_px) continue;

            // Midpoint
            const double mx = (static_cast<double>(p1.x) + static_cast<double>(p2.x)) * 0.5;
            const double my = (static_cast<double>(p1.y) + static_cast<double>(p2.y)) * 0.5;

            // Perpendicular unit direction: rotate (dx, dy) by 90 degrees
            const double px = -dy / d;
            const double py = dx / d;

            // Two points on the bisector line
            result.fst_x.push_back(mx - px);
            result.fst_y.push_back(my - py);
            result.snd_x.push_back(mx + px);
            result.snd_y.push_back(my + py);

            if (static_cast<int>(result.fst_x.size()) >= num_bisectors) {
                break;
            }
        }
    }

    return result;
}

/// Find the least-squares intersection of N lines.
/// Each line is defined by two points (fst, snd).
/// Reference: http://cal.cs.illinois.edu/~johannes/research/LS_line_intersec.pdf
///
/// For N lines with direction unit vectors n_i and points p_i:
///   R = sum(I - n_i * n_i^T)
///   q = sum((I - n_i * n_i^T) * p_i)
///   center = R^(-1) * q  (2x2 system)
cv::Point2d find_best_intersection(const BisectorLines& lines) {
    // Accumulate R (2x2) and q (2x1)
    double r00 = 0, r01 = 0, r10 = 0, r11 = 0;
    double q0 = 0, q1 = 0;

    const auto n = lines.fst_x.size();
    for (size_t i = 0; i < n; ++i) {
        // Direction vector (already unit length since fst/snd are +/- 1 from midpoint)
        double dx = lines.snd_x[i] - lines.fst_x[i];
        double dy = lines.snd_y[i] - lines.fst_y[i];
        const double len = std::hypot(dx, dy);
        dx /= len;
        dy /= len;

        // Projection matrix: I - n*n^T
        const double p00 = 1.0 - dx * dx;
        const double p01 = -dx * dy;
        const double p10 = -dy * dx;
        const double p11 = 1.0 - dy * dy;

        r00 += p00;
        r01 += p01;
        r10 += p10;
        r11 += p11;

        // q += P * first_point
        q0 += p00 * lines.fst_x[i] + p01 * lines.fst_y[i];
        q1 += p10 * lines.fst_x[i] + p11 * lines.fst_y[i];
    }

    // Solve 2x2 system: R * center = q
    const double det = r00 * r11 - r01 * r10;
    if (std::abs(det) < 1e-12) {
        return {0.0, 0.0};  // Degenerate
    }

    return {(r11 * q0 - r01 * q1) / det,
            (-r10 * q0 + r00 * q1) / det};
}

/// Estimate center of a single polygon using bisectors method.
cv::Point2d estimate_center(const Contour& polygon, int num_bisectors,
                             double min_dist_fraction, int max_iterations) {
    const double diameter = estimate_diameter(polygon);
    const double min_dist_px = min_dist_fraction * diameter;

    auto bisectors = calculate_perpendicular_bisectors(
        polygon, num_bisectors, min_dist_px, max_iterations);

    if (bisectors.fst_x.empty()) {
        // Fallback: centroid
        double sx = 0, sy = 0;
        for (const auto& pt : polygon.points) {
            sx += static_cast<double>(pt.x);
            sy += static_cast<double>(pt.y);
        }
        const auto n = static_cast<double>(polygon.points.size());
        return {sx / n, sy / n};
    }

    return find_best_intersection(bisectors);
}

}  // namespace

Result<EyeCenters> BisectorsEyeCenter::run(
    const GeometryPolygons& polygons) const {
    if (polygons.iris.points.size() < 3) {
        return make_error(ErrorCode::GeometryEstimationFailed,
                          "Iris contour has fewer than 3 points");
    }

    EyeCenters centers;

    centers.iris_center = estimate_center(
        polygons.iris, params_.num_bisectors,
        params_.min_distance_between_sector_points,
        params_.max_iterations);

    if (polygons.pupil.points.size() >= 3) {
        centers.pupil_center = estimate_center(
            polygons.pupil, params_.num_bisectors,
            params_.min_distance_between_sector_points,
            params_.max_iterations);
    } else {
        // Fallback: use iris center
        centers.pupil_center = centers.iris_center;
    }

    return centers;
}

IRIS_REGISTER_NODE("iris.BisectorsMethod", BisectorsEyeCenter);

}  // namespace iris
