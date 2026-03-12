#include <iris/nodes/specular_reflection_detector.hpp>

#include <opencv2/imgproc.hpp>

#include <iris/core/node_registry.hpp>

namespace iris {

SpecularReflectionDetector::SpecularReflectionDetector(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("reflection_threshold");
    if (it != node_params.end()) {
        params_.reflection_threshold = std::stoi(it->second);
    }
}

Result<NoiseMask> SpecularReflectionDetector::run(const IRImage& image) const {
    if (image.data.empty()) {
        return make_error(ErrorCode::ValidationFailed, "Empty input image");
    }

    cv::Mat thresholded;
    cv::threshold(image.data, thresholded,
                  static_cast<double>(params_.reflection_threshold),
                  255, cv::THRESH_BINARY);

    // Convert from 0/255 to 0/1 binary mask
    cv::Mat mask;
    thresholded.convertTo(mask, CV_8UC1, 1.0 / 255.0);

    return NoiseMask{.mask = mask};
}

IRIS_REGISTER_NODE("iris.SpecularReflectionDetection", SpecularReflectionDetector);

}  // namespace iris
