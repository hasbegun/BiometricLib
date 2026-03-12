#pragma once

#include <cmath>
#include <utility>
#include <vector>

#include <opencv2/core.hpp>

#include <iris/core/types.hpp>

namespace iris {

/// Convert cartesian coordinates to polar (rho, phi).
/// phi is in [0, 2*pi) matching Python's np.angle() % (2*pi).
std::pair<std::vector<double>, std::vector<double>>
cartesian2polar(const std::vector<double>& xs, const std::vector<double>& ys,
                double cx, double cy);

/// Overload for Contour (cv::Point2f vector).
std::pair<std::vector<double>, std::vector<double>>
cartesian2polar(const Contour& contour, double cx, double cy);

/// Convert polar coordinates back to cartesian.
std::pair<std::vector<double>, std::vector<double>>
polar2cartesian(const std::vector<double>& rhos, const std::vector<double>& phis,
                double cx, double cy);

/// Convert polar coordinates to a Contour.
Contour polar2contour(const std::vector<double>& rhos,
                      const std::vector<double>& phis,
                      double cx, double cy);

/// Polygon area via Shoelace formula.
double polygon_area(const Contour& contour);

/// Max pairwise distance between contour points.
double estimate_diameter(const Contour& contour);

/// Filled eyeball mask: pupil | iris | eyeball.
cv::Mat filled_eyeball_mask(const GeometryMask& m);

/// Filled iris mask: pupil | iris.
cv::Mat filled_iris_mask(const GeometryMask& m);

/// Fill a polygon contour into a binary mask of given size.
/// Returns CV_8UC1 with 1 inside the polygon, 0 outside.
cv::Mat contour_to_filled_mask(const Contour& contour, cv::Size mask_size);

/// Total perimeter length of a closed polygon.
double polygon_length(const Contour& contour);

}  // namespace iris
