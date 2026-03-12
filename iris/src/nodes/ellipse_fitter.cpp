#include <iris/nodes/ellipse_fitter.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <vector>

#include <opencv2/imgproc.hpp>

#include <iris/core/node_registry.hpp>

namespace iris {

EllipseFitter::EllipseFitter(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("dphi");
    if (it != node_params.end()) {
        params_.dphi = std::stod(it->second);
    }
}

namespace {

constexpr double kTwoPi = 2.0 * std::numbers::pi;
constexpr double kDegToRad = std::numbers::pi / 180.0;

/// Generate parametric ellipse points.
/// a, b = semi-axes, (x0, y0) = center, theta = rotation angle (radians)
Contour parametric_ellipse(double a, double b, double x0, double y0,
                            double theta, int nb_steps) {
    Contour result;
    result.points.reserve(static_cast<size_t>(nb_steps));

    const double sin_theta = std::sin(-theta);
    const double cos_theta = std::cos(-theta);

    for (int i = 0; i < nb_steps; ++i) {
        // Match Python's np.linspace(0, 2*pi, nb_steps) which includes endpoint
        const double t = kTwoPi * static_cast<double>(i) /
                         static_cast<double>(nb_steps - 1);
        const double cos_t = std::cos(t);
        const double sin_t = std::sin(t);

        const double x = x0 + b * cos_t * sin_theta + a * sin_t * cos_theta;
        const double y = y0 + b * cos_t * cos_theta - a * sin_t * sin_theta;

        result.points.emplace_back(static_cast<float>(x),
                                    static_cast<float>(y));
    }

    return result;
}

/// Check if fitted pupil ellipse is inside iris ellipse.
/// Uses bounding circle approximation with 5% safety margin.
bool is_pupil_inside_iris(const cv::RotatedRect& pupil_ellipse,
                           const cv::RotatedRect& iris_ellipse) {
    const double pupil_max_radius =
        static_cast<double>(
            std::max(pupil_ellipse.size.width, pupil_ellipse.size.height)) /
        2.0;
    const double iris_min_radius =
        static_cast<double>(
            std::min(iris_ellipse.size.width, iris_ellipse.size.height)) /
        2.0;

    const double dx =
        static_cast<double>(pupil_ellipse.center.x - iris_ellipse.center.x);
    const double dy =
        static_cast<double>(pupil_ellipse.center.y - iris_ellipse.center.y);
    const double center_dist = std::hypot(dx, dy);

    return (center_dist + pupil_max_radius) < (iris_min_radius * 0.95);
}

/// Find index of closest point in `candidates` to `target`.
size_t find_correspondence(const cv::Point2f& target,
                            const std::vector<cv::Point2f>& candidates) {
    size_t best_idx = 0;
    double best_dist = std::numeric_limits<double>::max();

    for (size_t i = 0; i < candidates.size(); ++i) {
        const double dx =
            static_cast<double>(target.x - candidates[i].x);
        const double dy =
            static_cast<double>(target.y - candidates[i].y);
        const double d = dx * dx + dy * dy;
        if (d < best_dist) {
            best_dist = d;
            best_idx = i;
        }
    }

    return best_idx;
}

}  // namespace

Result<std::optional<GeometryPolygons>> EllipseFitter::run(
    const GeometryPolygons& polygons) const {
    // Need at least 5 points for cv::fitEllipse
    if (polygons.iris.points.size() < 5 || polygons.pupil.points.size() < 5) {
        return make_error(ErrorCode::GeometryEstimationFailed,
                          "Need at least 5 points for ellipse fitting");
    }

    // Fit ellipses
    const cv::RotatedRect iris_ellipse =
        cv::fitEllipse(polygons.iris.points);
    const cv::RotatedRect pupil_ellipse =
        cv::fitEllipse(polygons.pupil.points);

    // Validate pupil inside iris
    if (!is_pupil_inside_iris(pupil_ellipse, iris_ellipse)) {
        return std::optional<GeometryPolygons>{std::nullopt};
    }

    // Generate parametric ellipse points
    // Match Python's round(360 / dphi) — round to nearest, not truncate
    const int nb_steps = static_cast<int>(std::lround(360.0 / params_.dphi));

    // cv::fitEllipse returns: center, (width, height), angle
    // width/height are full axes, so semi-axes = /2
    // angle is in degrees
    Contour fitted_iris = parametric_ellipse(
        static_cast<double>(iris_ellipse.size.width) / 2.0,
        static_cast<double>(iris_ellipse.size.height) / 2.0,
        static_cast<double>(iris_ellipse.center.x),
        static_cast<double>(iris_ellipse.center.y),
        static_cast<double>(iris_ellipse.angle) * kDegToRad,
        nb_steps);

    Contour fitted_pupil = parametric_ellipse(
        static_cast<double>(pupil_ellipse.size.width) / 2.0,
        static_cast<double>(pupil_ellipse.size.height) / 2.0,
        static_cast<double>(pupil_ellipse.center.x),
        static_cast<double>(pupil_ellipse.center.y),
        static_cast<double>(pupil_ellipse.angle) * kDegToRad,
        nb_steps);

    // Post-process: roll and flip to align angular ordering.
    // Matches Python: np.flip(np.roll(arr, round((-theta - 90) / dphi)))
    // This ensures the contour starts at 0° and goes counterclockwise,
    // consistent with LinearExtrapolation output.
    auto roll_and_flip = [&](Contour& contour, double theta_deg) {
        const auto n = static_cast<int>(contour.points.size());
        if (n == 0) return;

        const int roll_amount = static_cast<int>(
            std::round((-theta_deg - 90.0) / params_.dphi));

        // np.roll(arr, k): result[i] = arr[(i - k) % n]
        const int k = ((roll_amount % n) + n) % n;

        std::vector<cv::Point2f> rolled(static_cast<size_t>(n));
        for (int i = 0; i < n; ++i) {
            rolled[static_cast<size_t>(i)] =
                contour.points[static_cast<size_t>(
                    ((i - k) % n + n) % n)];
        }

        // np.flip: reverse the array
        std::reverse(rolled.begin(), rolled.end());

        contour.points = std::move(rolled);
    };

    roll_and_flip(fitted_iris,
                  static_cast<double>(iris_ellipse.angle));
    roll_and_flip(fitted_pupil,
                  static_cast<double>(pupil_ellipse.angle));

    // Refinement: replace fitted pupil points with original points.
    // Must happen AFTER roll+flip (matching Python ordering).
    auto fitted_pupil_copy = fitted_pupil.points;
    for (const auto& original_pt : polygons.pupil.points) {
        auto idx = find_correspondence(original_pt, fitted_pupil_copy);
        fitted_pupil.points[idx] = original_pt;
    }

    GeometryPolygons result;
    result.iris = std::move(fitted_iris);
    result.pupil = std::move(fitted_pupil);
    result.eyeball = polygons.eyeball;

    return std::optional<GeometryPolygons>{std::move(result)};
}

IRIS_REGISTER_NODE("iris.LSQEllipseFitWithRefinement", EllipseFitter);

}  // namespace iris
