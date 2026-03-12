#include <iris/nodes/linear_normalization.hpp>

#include <cmath>
#include <cstddef>
#include <string>

#include <iris/core/node_registry.hpp>
#include <iris/utils/normalization_utils.hpp>

namespace iris {

LinearNormalization::LinearNormalization(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("res_in_r");
    if (it != node_params.end()) params_.res_in_r = std::stoi(it->second);

    it = node_params.find("oversat_threshold");
    if (it != node_params.end()) params_.oversat_threshold = std::stoi(it->second);
}

std::vector<std::vector<cv::Point>> LinearNormalization::generate_correspondences(
    const Contour& pupil_points, const Contour& iris_points) const {
    const auto num_pts = pupil_points.points.size();
    const int res = params_.res_in_r;

    std::vector<std::vector<cv::Point>> src_points(static_cast<size_t>(res));

    for (int r = 0; r < res; ++r) {
        const double t =
            static_cast<double>(r) / static_cast<double>(res - 1);
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

Result<NormalizedIris> LinearNormalization::run(
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

    auto [pupil_pts, iris_pts] = correct_orientation(
        extrapolated_contours.pupil, extrapolated_contours.iris,
        eye_orientation.angle);

    cv::Mat iris_mask = generate_iris_mask(extrapolated_contours, noise_mask.mask);

    // Mark oversaturated pixels as not-iris
    for (int y = 0; y < image.data.rows; ++y) {
        const auto* img_row = image.data.ptr<uint8_t>(y);
        auto* mask_row = iris_mask.ptr<uint8_t>(y);
        for (int x = 0; x < image.data.cols; ++x) {
            if (img_row[x] >= static_cast<uint8_t>(params_.oversat_threshold)) {
                mask_row[x] = 0;
            }
        }
    }

    auto src_points = generate_correspondences(pupil_pts, iris_pts);
    auto [norm_image, norm_mask] =
        normalize_all(image.data, iris_mask, src_points);

    return NormalizedIris{
        .normalized_image = std::move(norm_image),
        .normalized_mask = std::move(norm_mask),
    };
}

IRIS_REGISTER_NODE("iris.LinearNormalization", LinearNormalization);

}  // namespace iris
