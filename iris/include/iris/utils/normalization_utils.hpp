#pragma once

#include <utility>
#include <vector>

#include <opencv2/core.hpp>

#include <iris/core/types.hpp>

namespace iris {

/// Roll pupil and iris boundary points by eye orientation angle.
/// Matches Python np.roll with num_rotations = -round(degrees(angle) * n / 360).
std::pair<Contour, Contour> correct_orientation(
    const Contour& pupil_points, const Contour& iris_points,
    double eye_orientation_radians);

/// Generate iris mask from extrapolated contours and noise mask.
/// Result = fillPoly(iris) & fillPoly(eyeball) & ~noise_mask.
/// Returns CV_8UC1 with values 0 or 1.
cv::Mat generate_iris_mask(const GeometryPolygons& contours,
                           const cv::Mat& noise_mask);

/// Nearest-neighbor normalization mapping.
/// src_points[r][c] = (x, y) in original image for each (row, col) in output.
/// Returns (normalized_image CV_64FC1 [0,255], normalized_mask CV_8UC1).
/// Image values are uint8 pixel intensities stored as doubles, matching Python.
std::pair<cv::Mat, cv::Mat> normalize_all(
    const cv::Mat& image, const cv::Mat& iris_mask,
    const std::vector<std::vector<cv::Point>>& src_points);

/// Bilinear interpolation of pixel intensity at sub-pixel coordinates.
double interpolate_pixel_intensity(const cv::Mat& image, double pixel_x,
                                   double pixel_y);

/// Get pixel value with bounds check, returning default_val if out of bounds.
double get_pixel_or_default(const cv::Mat& image, int x, int y,
                            double default_val);

}  // namespace iris
