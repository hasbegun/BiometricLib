#include <iris/nodes/eccentricity_offgaze_estimation.hpp>

#include <algorithm>
#include <cmath>
#include <string>

#include <opencv2/imgproc.hpp>

#include <iris/core/node_registry.hpp>

namespace iris {

EccentricityOffgazeEstimation::EccentricityOffgazeEstimation(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("eccentricity_method");
    if (it != node_params.end()) params_.eccentricity_method = it->second;

    it = node_params.find("assembling_method");
    if (it != node_params.end()) params_.assembling_method = it->second;
}

double EccentricityOffgazeEstimation::eccentricity_via_moments(
    const Contour& shape) {
    // cv::moments requires vector<Point2f>
    const auto moments = cv::moments(shape.points);

    constexpr double eps = 1e-10;
    const double moment_sum = moments.mu20 + moments.mu02;

    if (std::abs(moment_sum) < eps) return 1.0;

    // Eccentricity formula from moments
    const double diff = moments.mu20 - moments.mu02;
    return (diff * diff + 4.0 * moments.mu11 * moments.mu11) /
           (moment_sum * moment_sum);
}

double EccentricityOffgazeEstimation::eccentricity_via_ellipse_fit(
    const Contour& shape) {
    if (shape.points.size() < 5) return 0.0;

    const auto ellipse = cv::fitEllipse(shape.points);
    const double a = static_cast<double>(
        std::max(ellipse.size.width, ellipse.size.height));
    const double b = static_cast<double>(
        std::min(ellipse.size.width, ellipse.size.height));

    if (a < 1e-10) return 0.0;
    return std::sqrt(1.0 - (b / a) * (b / a));
}

Result<Offgaze> EccentricityOffgazeEstimation::run(
    const GeometryPolygons& geometries) const {
    if (geometries.pupil.points.empty() || geometries.iris.points.empty()) {
        return make_error(ErrorCode::GeometryEstimationFailed,
                          "Empty pupil or iris contour for offgaze");
    }

    // Select eccentricity method
    double pupil_ecc = 0.0;
    double iris_ecc = 0.0;

    if (params_.eccentricity_method == "ellipse_fit") {
        pupil_ecc = eccentricity_via_ellipse_fit(geometries.pupil);
        iris_ecc = eccentricity_via_ellipse_fit(geometries.iris);
    } else {
        // default: moments
        pupil_ecc = eccentricity_via_moments(geometries.pupil);
        iris_ecc = eccentricity_via_moments(geometries.iris);
    }

    // Assemble
    double score = 0.0;
    if (params_.assembling_method == "max") {
        score = std::max(pupil_ecc, iris_ecc);
    } else if (params_.assembling_method == "mean") {
        score = (pupil_ecc + iris_ecc) / 2.0;
    } else if (params_.assembling_method == "only_pupil") {
        score = pupil_ecc;
    } else if (params_.assembling_method == "only_iris") {
        score = iris_ecc;
    } else {
        // default: min
        score = std::min(pupil_ecc, iris_ecc);
    }

    return Offgaze{.score = score};
}

IRIS_REGISTER_NODE("iris.EccentricityOffgazeEstimation",
                    EccentricityOffgazeEstimation);

}  // namespace iris
