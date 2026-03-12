#include <iris/nodes/eyeball_distance_filter.hpp>

#include <cmath>

#include <opencv2/imgproc.hpp>

#include <iris/core/node_registry.hpp>
#include <iris/utils/geometry_utils.hpp>

namespace iris {

EyeballDistanceFilter::EyeballDistanceFilter(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("min_distance_to_noise_and_eyeball");
    if (it != node_params.end()) {
        params_.min_distance_to_noise_and_eyeball = std::stod(it->second);
    }
}

namespace {

/// Filter contour points, keeping only those not in the forbidden zone.
Contour filter_points(const Contour& contour, const cv::Mat& forbidden) {
    Contour result;
    result.points.reserve(contour.points.size());

    for (const auto& pt : contour.points) {
        const int x = static_cast<int>(std::round(pt.x));
        const int y = static_cast<int>(std::round(pt.y));

        // Bounds check
        if (y < 0 || y >= forbidden.rows || x < 0 || x >= forbidden.cols) {
            continue;
        }

        if (forbidden.at<uint8_t>(y, x) == 0) {
            result.points.push_back(pt);
        }
    }

    return result;
}

}  // namespace

Result<GeometryPolygons> EyeballDistanceFilter::run(
    const GeometryPolygons& polygons,
    const NoiseMask& noise_mask) const {
    if (polygons.iris.points.empty()) {
        return make_error(ErrorCode::VectorizationFailed,
                          "Empty iris contour — cannot filter");
    }

    const double iris_diam = estimate_diameter(polygons.iris);
    const int min_dist_px = static_cast<int>(
        std::round(params_.min_distance_to_noise_and_eyeball * iris_diam));

    // Create combined noise mask: existing noise + eyeball contour points
    cv::Mat combined = noise_mask.mask.clone();
    if (combined.empty()) {
        // If no noise mask provided, create a zero mask
        if (!polygons.iris.points.empty()) {
            // Infer image size from contour bounds
            cv::Rect bounds = cv::boundingRect(polygons.eyeball.points);
            combined = cv::Mat::zeros(bounds.height + bounds.y + 10,
                                      bounds.width + bounds.x + 10, CV_8UC1);
        } else {
            return polygons;  // nothing to filter
        }
    }

    // Paint eyeball contour points into the combined mask
    for (const auto& pt : polygons.eyeball.points) {
        const int x = static_cast<int>(std::round(pt.x));
        const int y = static_cast<int>(std::round(pt.y));
        if (y >= 0 && y < combined.rows && x >= 0 && x < combined.cols) {
            combined.at<uint8_t>(y, x) = 1;
        }
    }

    // Dilate the combined mask using box blur.
    // Must blur on float to match Python: cv2.blur(mask.astype(float), ...).astype(bool)
    // Blurring uint8 {0,1} loses small averages (e.g., 1/9 ≈ 0.11 → uint8 = 0),
    // making the forbidden zone much too small.
    const int kernel_size = 2 * min_dist_px + 1;
    cv::Mat forbidden;
    if (kernel_size > 1) {
        cv::Mat float_combined;
        combined.convertTo(float_combined, CV_64FC1);
        cv::Mat float_forbidden;
        cv::blur(float_combined, float_forbidden,
                 cv::Size(kernel_size, kernel_size));
        // Any non-zero → forbidden (matches Python's astype(bool))
        cv::threshold(float_forbidden, forbidden, 0, 1, cv::THRESH_BINARY);
        forbidden.convertTo(forbidden, CV_8UC1);
    } else {
        forbidden = combined;
    }

    // Filter iris and pupil points. Eyeball passes through unchanged.
    GeometryPolygons result;
    result.iris = filter_points(polygons.iris, forbidden);
    result.pupil = filter_points(polygons.pupil, forbidden);
    result.eyeball = polygons.eyeball;

    if (result.iris.points.empty()) {
        return make_error(ErrorCode::VectorizationFailed,
                          "All iris contour points filtered — increase tolerance");
    }

    return result;
}

IRIS_REGISTER_NODE("iris.ContourPointNoiseEyeballDistanceFilter", EyeballDistanceFilter);

}  // namespace iris
