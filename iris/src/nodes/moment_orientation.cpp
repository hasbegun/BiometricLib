#include <iris/nodes/moment_orientation.hpp>

#include <cmath>
#include <numbers>
#include <vector>

#include <opencv2/imgproc.hpp>

#include <iris/core/node_registry.hpp>

namespace iris {

MomentOrientation::MomentOrientation(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("eccentricity_threshold");
    if (it != node_params.end()) {
        params_.eccentricity_threshold = std::stod(it->second);
    }
}

Result<EyeOrientation> MomentOrientation::run(
    const GeometryPolygons& polygons) const {
    if (polygons.eyeball.points.size() < 3) {
        return make_error(ErrorCode::GeometryEstimationFailed,
                          "Eyeball contour has fewer than 3 points");
    }

    // Convert to cv::Point2f vector for cv::moments
    std::vector<cv::Point2f> pts(polygons.eyeball.points.begin(),
                                  polygons.eyeball.points.end());

    const auto moments = cv::moments(pts);

    // Compute eccentricity
    const double mu20 = moments.mu20;
    const double mu02 = moments.mu02;
    const double mu11 = moments.mu11;
    const double moment_sum = mu20 + mu02;

    if (std::abs(moment_sum) < 1e-12) {
        return make_error(ErrorCode::GeometryEstimationFailed,
                          "Degenerate eyeball shape — zero moment sum");
    }

    const double eccentricity =
        ((mu20 - mu02) * (mu20 - mu02) + 4.0 * mu11 * mu11) /
        (moment_sum * moment_sum);

    if (eccentricity < params_.eccentricity_threshold) {
        return make_error(
            ErrorCode::GeometryEstimationFailed,
            "Eyeball too circular (eccentricity " +
                std::to_string(eccentricity) + " < " +
                std::to_string(params_.eccentricity_threshold) + ")");
    }

    // Orientation: angle of principal axis
    constexpr double kPi = std::numbers::pi;
    double angle = 0.5 * std::atan2(2.0 * mu11, mu20 - mu02);

    // Normalize to [-pi/2, pi/2)
    angle = std::fmod(angle + kPi / 2.0, kPi) - kPi / 2.0;

    return EyeOrientation{.angle = angle};
}

IRIS_REGISTER_NODE("iris.MomentOfArea", MomentOrientation);

}  // namespace iris
