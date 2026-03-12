#pragma once

#include <utility>

#include <opencv2/core.hpp>

namespace iris::filter_utils {

/// Create centered coordinate grids for filter kernels.
/// Returns (x, y) where x has shape (rho_size, phi_size) varying along rows,
/// and y has shape (rho_size, phi_size) varying along columns.
/// Matches Python: y, x = np.meshgrid(arange(-phi_half, phi_half+1),
///                                     arange(-rho_half, rho_half+1), indexing="xy")
std::pair<cv::Mat, cv::Mat> make_meshgrid(int phi_size, int rho_size);

/// Rotate coordinates by angle (in degrees).
/// rotx =  x * cos(θ) + y * sin(θ)
/// roty = -x * sin(θ) + y * cos(θ)
std::pair<cv::Mat, cv::Mat> rotate_coords(const cv::Mat& x, const cv::Mat& y,
                                           double angle_degrees);

/// Compute element-wise radius: sqrt(x^2 + y^2).
cv::Mat compute_radius(const cv::Mat& x, const cv::Mat& y);

/// FFT shift: swap quadrants (top-left ↔ bottom-right, top-right ↔ bottom-left).
cv::Mat fftshift(const cv::Mat& src);

/// Normalize real and imaginary parts so each has Frobenius norm 1.0.
void normalize_frobenius(cv::Mat& real, cv::Mat& imag);

/// Convert to fixed-point: multiply by 2^15 and round.
void apply_fixpoints(cv::Mat& real, cv::Mat& imag);

}  // namespace iris::filter_utils
