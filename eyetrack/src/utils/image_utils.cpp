#include <eyetrack/utils/image_utils.hpp>

#include <algorithm>

#include <opencv2/imgproc.hpp>

namespace eyetrack {

Result<cv::Mat> safe_crop(const cv::Mat& image, cv::Rect roi) {
    if (image.empty()) {
        return make_error(ErrorCode::InvalidInput, "Empty image for safe_crop");
    }

    // Clip ROI to image bounds
    int x1 = std::max(0, roi.x);
    int y1 = std::max(0, roi.y);
    int x2 = std::min(image.cols, roi.x + roi.width);
    int y2 = std::min(image.rows, roi.y + roi.height);

    if (x2 <= x1 || y2 <= y1) {
        return make_error(ErrorCode::InvalidInput,
                          "Crop region has zero area after clipping");
    }

    cv::Rect clipped(x1, y1, x2 - x1, y2 - y1);
    return image(clipped).clone();
}

Result<cv::Mat> resolution_scale(const cv::Mat& image, double factor) {
    if (image.empty()) {
        return make_error(ErrorCode::InvalidInput, "Empty image for resolution_scale");
    }
    if (factor <= 0.0) {
        return make_error(ErrorCode::InvalidInput, "Scale factor must be positive");
    }

    cv::Mat result;
    cv::resize(image, result, cv::Size(), factor, factor, cv::INTER_LINEAR);
    return result;
}

Result<cv::Mat> bgr_to_rgb(const cv::Mat& image) {
    if (image.empty()) {
        return make_error(ErrorCode::InvalidInput, "Empty image for bgr_to_rgb");
    }
    if (image.channels() != 3) {
        return make_error(ErrorCode::InvalidInput,
                          "bgr_to_rgb requires 3-channel input");
    }

    cv::Mat result;
    cv::cvtColor(image, result, cv::COLOR_BGR2RGB);
    return result;
}

Result<cv::Mat> grayscale_to_bgr(const cv::Mat& image) {
    if (image.empty()) {
        return make_error(ErrorCode::InvalidInput, "Empty image for grayscale_to_bgr");
    }
    if (image.channels() != 1) {
        return make_error(ErrorCode::InvalidInput,
                          "grayscale_to_bgr requires single-channel input");
    }

    cv::Mat result;
    cv::cvtColor(image, result, cv::COLOR_GRAY2BGR);
    return result;
}

}  // namespace eyetrack
