#include <iris/nodes/sharpness_estimation.hpp>

#include <cmath>
#include <string>

#include <opencv2/imgproc.hpp>

#include <iris/core/node_registry.hpp>

namespace iris {

SharpnessEstimation::SharpnessEstimation(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("lap_ksize");
    if (it != node_params.end()) params_.lap_ksize = std::stoi(it->second);

    it = node_params.find("erosion_ksize_w");
    if (it != node_params.end()) params_.erosion_ksize_w = std::stoi(it->second);

    it = node_params.find("erosion_ksize_h");
    if (it != node_params.end()) params_.erosion_ksize_h = std::stoi(it->second);
}

Result<Sharpness> SharpnessEstimation::run(
    const NormalizedIris& normalization_output) const {
    if (normalization_output.normalized_image.empty()) {
        return make_error(ErrorCode::GeometryEstimationFailed,
                          "Empty normalized image for sharpness");
    }

    // Python: cv2.Laplacian(image / 255, cv2.CV_32F, ksize=lap_ksize)
    // normalized_image is CV_64FC1 in [0,255]; divide by 255 to match Python
    cv::Mat scaled_image;
    normalization_output.normalized_image.convertTo(scaled_image, CV_64F,
                                                    1.0 / 255.0);
    cv::Mat laplacian;
    cv::Laplacian(scaled_image, laplacian, CV_64F, params_.lap_ksize);

    // Erode the mask
    cv::Mat eroded_mask;
    cv::Mat kernel = cv::Mat::ones(params_.erosion_ksize_h,
                                   params_.erosion_ksize_w, CV_8UC1);
    cv::erode(normalization_output.normalized_mask, eroded_mask, kernel);

    // Compute std of Laplacian in unmasked region
    const int valid_count = cv::countNonZero(eroded_mask);
    if (valid_count == 0) {
        return Sharpness{.score = 0.0};
    }

    // Compute mean and std within mask
    double sum = 0.0;
    double sum_sq = 0.0;
    int count = 0;

    for (int y = 0; y < laplacian.rows; ++y) {
        const auto* lap_row = laplacian.ptr<double>(y);
        const auto* mask_row = eroded_mask.ptr<uint8_t>(y);
        for (int x = 0; x < laplacian.cols; ++x) {
            if (mask_row[x] != 0) {
                const double v = lap_row[x];
                sum += v;
                sum_sq += v * v;
                ++count;
            }
        }
    }

    const double mean = sum / static_cast<double>(count);
    const double variance =
        sum_sq / static_cast<double>(count) - mean * mean;
    const double std_dev = std::sqrt(std::max(variance, 0.0));

    return Sharpness{.score = std_dev};
}

IRIS_REGISTER_NODE("iris.SharpnessEstimation", SharpnessEstimation);

}  // namespace iris
