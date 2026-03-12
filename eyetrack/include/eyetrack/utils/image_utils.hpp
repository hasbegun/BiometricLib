#pragma once

#include <opencv2/core.hpp>

#include <eyetrack/core/error.hpp>

namespace eyetrack {

/// Safely crop a region from an image with bounds checking.
/// The requested ROI is clipped to the image boundaries.
/// Returns an error if the resulting crop has zero area or the input is empty.
[[nodiscard]] Result<cv::Mat> safe_crop(const cv::Mat& image, cv::Rect roi);

/// Scale an image by the given factor using bilinear interpolation.
/// factor > 1 upscales, factor < 1 downscales.
/// Returns an error if the input is empty or factor is non-positive.
[[nodiscard]] Result<cv::Mat> resolution_scale(const cv::Mat& image, double factor);

/// Convert BGR image to RGB (channel swap).
/// Returns an error if the input is empty or not 3-channel.
[[nodiscard]] Result<cv::Mat> bgr_to_rgb(const cv::Mat& image);

/// Convert single-channel grayscale image to 3-channel BGR.
/// Returns an error if the input is empty or not single-channel.
[[nodiscard]] Result<cv::Mat> grayscale_to_bgr(const cv::Mat& image);

}  // namespace eyetrack
