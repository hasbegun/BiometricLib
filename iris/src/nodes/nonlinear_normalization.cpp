#include <iris/nodes/nonlinear_normalization.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

#include <iris/core/node_registry.hpp>
#include <iris/utils/geometry_utils.hpp>
#include <iris/utils/normalization_utils.hpp>

namespace iris {

NonlinearNormalization::NonlinearNormalization(const Params& params)
    : params_(params) {
    precompute_grids();
}

NonlinearNormalization::NonlinearNormalization(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("res_in_r");
    if (it != node_params.end()) params_.res_in_r = std::stoi(it->second);

    it = node_params.find("oversat_threshold");
    if (it != node_params.end()) params_.oversat_threshold = std::stoi(it->second);

    precompute_grids();
}

void NonlinearNormalization::precompute_grids() {
    intermediate_radiuses_.resize(101);
    for (int p = 0; p <= 100; ++p) {
        intermediate_radiuses_[static_cast<size_t>(p)] =
            compute_grid(params_.res_in_r, p);
    }
}

std::vector<double> NonlinearNormalization::compute_grid(
    int res_in_r, int p2i_ratio_percent) {
    // Python: p = np.arange(28, max(74 - P_r100, P_r100 - 14)) ** 2
    const int upper = std::max(74 - p2i_ratio_percent, p2i_ratio_percent - 14);
    if (upper <= 28) {
        // Degenerate case: fall back to linear
        std::vector<double> result(static_cast<size_t>(res_in_r));
        for (int i = 0; i < res_in_r; ++i) {
            result[static_cast<size_t>(i)] =
                static_cast<double>(i) / static_cast<double>(res_in_r - 1);
        }
        return result;
    }

    // Build p array: arange(28, upper)^2
    const int p_len = upper - 28;
    std::vector<double> p_arr(static_cast<size_t>(p_len));
    for (int i = 0; i < p_len; ++i) {
        const double val = static_cast<double>(28 + i);
        p_arr[static_cast<size_t>(i)] = val * val;
    }

    // Normalize q: (p - p[0]) / (p[-1] - p[0])
    const double p_first = p_arr[0];
    const double p_last = p_arr[static_cast<size_t>(p_len - 1)];
    const double p_range = p_last - p_first;

    std::vector<double> q(static_cast<size_t>(p_len));
    for (int i = 0; i < p_len; ++i) {
        q[static_cast<size_t>(i)] =
            (p_range > 0.0) ? (p_arr[static_cast<size_t>(i)] - p_first) / p_range
                            : 0.0;
    }

    // Interpolate: grids = interp(linspace(0,1,res+1), linspace(0,1,len(q)), q)
    const int grid_len = res_in_r + 1;
    std::vector<double> grids(static_cast<size_t>(grid_len));
    for (int i = 0; i < grid_len; ++i) {
        const double x = static_cast<double>(i) / static_cast<double>(grid_len - 1);
        // Find position in q's x-axis (linspace(0,1,p_len))
        const double qx = x * static_cast<double>(p_len - 1);
        const int idx = static_cast<int>(std::floor(qx));
        const double frac = qx - static_cast<double>(idx);

        if (idx >= p_len - 1) {
            grids[static_cast<size_t>(i)] = q[static_cast<size_t>(p_len - 1)];
        } else {
            grids[static_cast<size_t>(i)] =
                q[static_cast<size_t>(idx)] * (1.0 - frac) +
                q[static_cast<size_t>(idx + 1)] * frac;
        }
    }

    // Return midpoints: grids[:-1] + diff(grids) / 2
    std::vector<double> result(static_cast<size_t>(res_in_r));
    for (int i = 0; i < res_in_r; ++i) {
        result[static_cast<size_t>(i)] =
            (grids[static_cast<size_t>(i)] +
             grids[static_cast<size_t>(i + 1)]) / 2.0;
    }

    return result;
}

std::vector<std::vector<cv::Point>> NonlinearNormalization::generate_correspondences(
    const Contour& pupil_points, const Contour& iris_points,
    double p2i_ratio) const {
    const int p2i_pct = static_cast<int>(
        std::round(100.0 * p2i_ratio));
    const int clamped_pct = std::clamp(p2i_pct, 0, 100);
    const auto& grid = intermediate_radiuses_[static_cast<size_t>(clamped_pct)];
    const auto num_pts = pupil_points.points.size();
    const int res = params_.res_in_r;

    std::vector<std::vector<cv::Point>> src_points(static_cast<size_t>(res));

    for (int r = 0; r < res; ++r) {
        const double t = grid[static_cast<size_t>(r)];
        auto& row = src_points[static_cast<size_t>(r)];
        row.reserve(num_pts);

        for (size_t c = 0; c < num_pts; ++c) {
            const double px = static_cast<double>(pupil_points.points[c].x);
            const double py = static_cast<double>(pupil_points.points[c].y);
            const double ix = static_cast<double>(iris_points.points[c].x);
            const double iy = static_cast<double>(iris_points.points[c].y);

            const int x = static_cast<int>(
                std::round(px + t * (ix - px)));
            const int y = static_cast<int>(
                std::round(py + t * (iy - py)));
            row.emplace_back(x, y);
        }
    }

    return src_points;
}

Result<NormalizedIris> NonlinearNormalization::run(
    const IRImage& image,
    const NoiseMask& noise_mask,
    const GeometryPolygons& extrapolated_contours,
    const EyeOrientation& eye_orientation) const {
    if (extrapolated_contours.pupil.points.size() !=
        extrapolated_contours.iris.points.size()) {
        return make_error(ErrorCode::NormalizationFailed,
                          "Pupil and iris point counts differ");
    }
    if (extrapolated_contours.pupil.points.empty()) {
        return make_error(ErrorCode::NormalizationFailed,
                          "Empty contour points");
    }

    const double pupil_d = estimate_diameter(extrapolated_contours.pupil);
    const double iris_d = estimate_diameter(extrapolated_contours.iris);

    if (iris_d <= 0.0 || pupil_d <= 0.0 || pupil_d >= iris_d) {
        return make_error(ErrorCode::NormalizationFailed,
                          "Invalid pupil-to-iris ratio");
    }
    const double p2i_ratio = pupil_d / iris_d;

    auto [pupil_pts, iris_pts] = correct_orientation(
        extrapolated_contours.pupil, extrapolated_contours.iris,
        eye_orientation.angle);

    cv::Mat iris_mask = generate_iris_mask(extrapolated_contours, noise_mask.mask);

    for (int y = 0; y < image.data.rows; ++y) {
        const auto* img_row = image.data.ptr<uint8_t>(y);
        auto* mask_row = iris_mask.ptr<uint8_t>(y);
        for (int x = 0; x < image.data.cols; ++x) {
            if (img_row[x] >= static_cast<uint8_t>(params_.oversat_threshold)) {
                mask_row[x] = 0;
            }
        }
    }

    auto src_points = generate_correspondences(pupil_pts, iris_pts, p2i_ratio);
    auto [norm_image, norm_mask] =
        normalize_all(image.data, iris_mask, src_points);

    return NormalizedIris{
        .normalized_image = std::move(norm_image),
        .normalized_mask = std::move(norm_mask),
    };
}

IRIS_REGISTER_NODE("iris.NonlinearNormalization", NonlinearNormalization);

}  // namespace iris
