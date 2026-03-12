#include <iris/utils/geometry_utils.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

#include <opencv2/imgproc.hpp>

namespace iris {

namespace {
constexpr double kTwoPi = 2.0 * std::numbers::pi;
}  // namespace

std::pair<std::vector<double>, std::vector<double>>
cartesian2polar(const std::vector<double>& xs, const std::vector<double>& ys,
                double cx, double cy) {
    const size_t n = xs.size();
    std::vector<double> rhos(n);
    std::vector<double> phis(n);

    for (size_t i = 0; i < n; ++i) {
        const double dx = xs[i] - cx;
        const double dy = ys[i] - cy;
        rhos[i] = std::hypot(dx, dy);
        double phi = std::atan2(dy, dx);
        if (phi < 0.0) {
            phi += kTwoPi;
        }
        phis[i] = phi;
    }

    return {rhos, phis};
}

std::pair<std::vector<double>, std::vector<double>>
cartesian2polar(const Contour& contour, double cx, double cy) {
    const size_t n = contour.points.size();
    std::vector<double> xs(n);
    std::vector<double> ys(n);
    for (size_t i = 0; i < n; ++i) {
        xs[i] = static_cast<double>(contour.points[i].x);
        ys[i] = static_cast<double>(contour.points[i].y);
    }
    return cartesian2polar(xs, ys, cx, cy);
}

std::pair<std::vector<double>, std::vector<double>>
polar2cartesian(const std::vector<double>& rhos, const std::vector<double>& phis,
                double cx, double cy) {
    const size_t n = rhos.size();
    std::vector<double> xs(n);
    std::vector<double> ys(n);

    for (size_t i = 0; i < n; ++i) {
        xs[i] = cx + rhos[i] * std::cos(phis[i]);
        ys[i] = cy + rhos[i] * std::sin(phis[i]);
    }

    return {xs, ys};
}

Contour polar2contour(const std::vector<double>& rhos,
                      const std::vector<double>& phis,
                      double cx, double cy) {
    auto [xs, ys] = polar2cartesian(rhos, phis, cx, cy);
    Contour result;
    result.points.reserve(xs.size());
    for (size_t i = 0; i < xs.size(); ++i) {
        result.points.emplace_back(static_cast<float>(xs[i]),
                                   static_cast<float>(ys[i]));
    }
    return result;
}

double polygon_area(const Contour& contour) {
    const auto& pts = contour.points;
    const size_t n = pts.size();
    if (n < 3) return 0.0;

    double area = 0.0;
    for (size_t i = 0; i < n; ++i) {
        const size_t j = (i + 1) % n;
        area += static_cast<double>(pts[i].x) * static_cast<double>(pts[j].y);
        area -= static_cast<double>(pts[j].x) * static_cast<double>(pts[i].y);
    }

    return std::abs(area) * 0.5;
}

double estimate_diameter(const Contour& contour) {
    const auto& pts = contour.points;
    const size_t n = pts.size();
    if (n < 2) return 0.0;

    double max_dist_sq = 0.0;
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            const double dx = static_cast<double>(pts[i].x - pts[j].x);
            const double dy = static_cast<double>(pts[i].y - pts[j].y);
            max_dist_sq = std::max(max_dist_sq, dx * dx + dy * dy);
        }
    }

    return std::sqrt(max_dist_sq);
}

cv::Mat filled_eyeball_mask(const GeometryMask& m) {
    cv::Mat result;
    cv::bitwise_or(m.pupil, m.iris, result);
    cv::bitwise_or(result, m.eyeball, result);
    return result;
}

cv::Mat filled_iris_mask(const GeometryMask& m) {
    cv::Mat result;
    cv::bitwise_or(m.pupil, m.iris, result);
    return result;
}

cv::Mat contour_to_filled_mask(const Contour& contour, cv::Size mask_size) {
    cv::Mat mask = cv::Mat::zeros(mask_size, CV_8UC1);
    if (contour.points.size() < 3) return mask;

    std::vector<cv::Point> pts;
    pts.reserve(contour.points.size());
    for (const auto& p : contour.points) {
        pts.emplace_back(static_cast<int>(std::round(static_cast<double>(p.x))),
                         static_cast<int>(std::round(static_cast<double>(p.y))));
    }
    const std::vector<std::vector<cv::Point>> polys = {pts};
    cv::fillPoly(mask, polys, cv::Scalar(1));
    return mask;
}

double polygon_length(const Contour& contour) {
    const auto& pts = contour.points;
    const size_t n = pts.size();
    if (n < 2) return 0.0;

    double length = 0.0;
    for (size_t i = 0; i < n; ++i) {
        const size_t j = (i + 1) % n;
        const double dx = static_cast<double>(pts[j].x) - static_cast<double>(pts[i].x);
        const double dy = static_cast<double>(pts[j].y) - static_cast<double>(pts[i].y);
        length += std::hypot(dx, dy);
    }
    return length;
}

}  // namespace iris
