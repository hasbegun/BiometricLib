#include <iris/nodes/segmentation_binarizer.hpp>

#include <opencv2/core.hpp>

#include <iris/core/node_registry.hpp>

namespace iris {

SegmentationBinarizer::SegmentationBinarizer(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto get = [&](const std::string& key, double& out) {
        auto it = node_params.find(key);
        if (it != node_params.end()) {
            out = std::stod(it->second);
        }
    };
    get("eyeball_threshold", params_.eyeball_threshold);
    get("iris_threshold", params_.iris_threshold);
    get("pupil_threshold", params_.pupil_threshold);
    get("eyelashes_threshold", params_.eyelashes_threshold);
}

Result<std::pair<GeometryMask, NoiseMask>>
SegmentationBinarizer::run(const SegmentationMap& segmap) const {
    if (segmap.predictions.empty()) {
        return make_error(ErrorCode::SegmentationFailed, "Empty segmentation map");
    }

    // Channel order: 0=eyeball, 1=iris, 2=pupil, 3=eyelashes
    const int num_channels = segmap.predictions.channels();
    if (num_channels < 4) {
        return make_error(ErrorCode::SegmentationFailed,
                          "Segmentation map requires 4 channels",
                          "got " + std::to_string(num_channels));
    }

    // Split into per-class channels
    std::vector<cv::Mat> channels;
    cv::split(segmap.predictions, channels);

    // Threshold each class
    auto binarize = [](const cv::Mat& channel, double threshold) -> cv::Mat {
        cv::Mat result;
        cv::compare(channel, threshold, result, cv::CMP_GE);
        // cv::compare outputs 0 or 255; convert to 0/1
        result.convertTo(result, CV_8UC1, 1.0 / 255.0);
        return result;
    };

    GeometryMask geometry;
    geometry.eyeball = binarize(channels[0], params_.eyeball_threshold);
    geometry.iris = binarize(channels[1], params_.iris_threshold);
    geometry.pupil = binarize(channels[2], params_.pupil_threshold);

    NoiseMask noise;
    noise.mask = binarize(channels[3], params_.eyelashes_threshold);

    return std::pair{geometry, noise};
}

IRIS_REGISTER_NODE("iris.MultilabelSegmentationBinarization", SegmentationBinarizer);

}  // namespace iris
