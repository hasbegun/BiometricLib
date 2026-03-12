#include <iris/nodes/occlusion_calculator.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <string>

#include <opencv2/core.hpp>

#include <iris/core/node_registry.hpp>
#include <iris/utils/geometry_utils.hpp>

namespace iris {

OcclusionCalculator::OcclusionCalculator(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("quantile_angle");
    if (it != node_params.end()) params_.quantile_angle = std::stod(it->second);
}

Contour OcclusionCalculator::get_quantile_points(
    const Contour& iris_coords,
    const EyeOrientation& eye_orientation) const {
    const auto n = static_cast<int>(iris_coords.points.size());
    if (n == 0) return {};

    // Roll by orientation angle
    const double angle_degrees =
        eye_orientation.angle * 180.0 / std::numbers::pi;
    const int num_rotations =
        -static_cast<int>(std::round(angle_degrees * static_cast<double>(n) / 360.0));
    int shift = num_rotations % n;
    if (shift < 0) shift += n;

    auto rolled = [&](int idx) -> cv::Point2f {
        return iris_coords.points[static_cast<size_t>(
            (idx + shift) % n)];
    };

    // Scaled quantile: number of points per angular slice
    const int scaled_quantile = static_cast<int>(
        std::round(params_.quantile_angle * static_cast<double>(n) / 360.0));

    Contour result;
    // First slice: [0, scaled_quantile)
    for (int i = 0; i < scaled_quantile; ++i) {
        result.points.push_back(rolled(i));
    }
    // Middle slice: [n/2 - scaled_quantile, n/2 + scaled_quantile)
    const int half = n / 2;
    for (int i = half - scaled_quantile; i < half + scaled_quantile; ++i) {
        result.points.push_back(rolled(i));
    }
    // Last slice: [n - scaled_quantile, n)
    for (int i = n - scaled_quantile; i < n; ++i) {
        result.points.push_back(rolled(i));
    }

    return result;
}

Result<EyeOcclusion> OcclusionCalculator::run(
    const GeometryPolygons& extrapolated_polygons,
    const NoiseMask& noise_mask,
    const EyeOrientation& eye_orientation,
    const EyeCenters& /*eye_centers*/) const {
    if (params_.quantile_angle == 0.0) {
        return EyeOcclusion{.fraction = 0.0};
    }

    const int img_h = noise_mask.mask.rows;
    const int img_w = noise_mask.mask.cols;

    // Frame polygon = image boundary
    Contour frame;
    frame.points = {{0.0f, 0.0f},
                    {0.0f, static_cast<float>(img_h)},
                    {static_cast<float>(img_w), static_cast<float>(img_h)},
                    {static_cast<float>(img_w), 0.0f},
                    {0.0f, 0.0f}};

    // Compute bounding box of all points to determine offset/padding
    float all_min_x = 0.0f;
    float all_min_y = 0.0f;
    float all_max_x = static_cast<float>(img_w);
    float all_max_y = static_cast<float>(img_h);

    auto update_bounds = [&](const Contour& c) {
        for (const auto& p : c.points) {
            all_min_x = std::min(all_min_x, p.x);
            all_min_y = std::min(all_min_y, p.y);
            all_max_x = std::max(all_max_x, p.x);
            all_max_y = std::max(all_max_y, p.y);
        }
    };
    update_bounds(extrapolated_polygons.iris);
    update_bounds(extrapolated_polygons.eyeball);
    update_bounds(extrapolated_polygons.pupil);

    // Offset = floor(min), ensures no negative coords
    const int off_x = static_cast<int>(std::floor(static_cast<double>(all_min_x)));
    const int off_y = static_cast<int>(std::floor(static_cast<double>(all_min_y)));

    const int total_w = static_cast<int>(
        std::ceil(static_cast<double>(all_max_x))) - off_x;
    const int total_h = static_cast<int>(
        std::ceil(static_cast<double>(all_max_y))) - off_y;
    const cv::Size total_size(total_w, total_h);

    // Helper: offset a contour
    auto offset_contour = [&](const Contour& c) -> Contour {
        Contour result;
        result.points.reserve(c.points.size());
        for (const auto& p : c.points) {
            result.points.emplace_back(p.x - static_cast<float>(off_x),
                                       p.y - static_cast<float>(off_y));
        }
        return result;
    };

    // Pad noise mask
    const int pad_top = -off_y;
    const int pad_left = -off_x;
    const int overflow_x = static_cast<int>(
        std::ceil(static_cast<double>(all_max_x))) - img_w;
    const int overflow_y = static_cast<int>(
        std::ceil(static_cast<double>(all_max_y))) - img_h;
    const int pad_right = std::max(overflow_x, 0);
    const int pad_bottom = std::max(overflow_y, 0);

    cv::Mat padded_noise;
    cv::copyMakeBorder(noise_mask.mask, padded_noise,
                       std::max(pad_top, 0), pad_bottom,
                       std::max(pad_left, 0), pad_right,
                       cv::BORDER_CONSTANT, cv::Scalar(0));

    // Ensure padded_noise matches total_size
    if (padded_noise.size() != total_size) {
        padded_noise = padded_noise(cv::Rect(0, 0, total_w, total_h)).clone();
    }

    // Get quantile iris polygon
    Contour iris_quantile = get_quantile_points(
        extrapolated_polygons.iris, eye_orientation);

    // Create filled masks in offset space
    cv::Mat iris_mask_q = contour_to_filled_mask(
        offset_contour(iris_quantile), total_size);
    cv::Mat pupil_mask = contour_to_filled_mask(
        offset_contour(extrapolated_polygons.pupil), total_size);
    cv::Mat eyeball_mask = contour_to_filled_mask(
        offset_contour(extrapolated_polygons.eyeball), total_size);
    cv::Mat frame_mask = contour_to_filled_mask(
        offset_contour(frame), total_size);

    // visible_iris = iris_quantile & ~pupil & eyeball & ~noise & frame
    cv::Mat not_pupil;
    cv::bitwise_not(pupil_mask, not_pupil);
    cv::Mat not_noise;
    cv::bitwise_not(padded_noise, not_noise);

    cv::Mat visible_iris;
    cv::bitwise_and(iris_mask_q, not_pupil, visible_iris);
    cv::bitwise_and(visible_iris, eyeball_mask, visible_iris);
    cv::bitwise_and(visible_iris, not_noise, visible_iris);
    cv::bitwise_and(visible_iris, frame_mask, visible_iris);

    // extrapolated_iris = iris_quantile & ~pupil
    cv::Mat extrapolated_iris;
    cv::bitwise_and(iris_mask_q, not_pupil, extrapolated_iris);

    const int total_extrap = cv::countNonZero(extrapolated_iris);
    if (total_extrap == 0) {
        return EyeOcclusion{.fraction = 0.0};
    }

    const double visible_fraction =
        static_cast<double>(cv::countNonZero(visible_iris)) /
        static_cast<double>(total_extrap);

    return EyeOcclusion{.fraction = visible_fraction};
}

IRIS_REGISTER_NODE("iris.OcclusionCalculator", OcclusionCalculator);

}  // namespace iris
