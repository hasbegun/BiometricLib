#include <eyetrack/nodes/eye_extractor.hpp>

#include <algorithm>
#include <cmath>

#include <eyetrack/core/node_registry.hpp>

namespace eyetrack {

const std::vector<std::string> EyeExtractor::kDependencies = {"eyetrack.FaceDetector"};

EyeExtractor::EyeExtractor() = default;

EyeExtractor::EyeExtractor(const Config& config) : config_(config) {}

EyeExtractor::EyeExtractor(const NodeParams& params) {
    if (auto it = params.find("padding"); it != params.end())
        config_.padding = std::stof(it->second);
}

std::array<Point2D, 6> EyeExtractor::extract_ear(
    const std::vector<Point2D>& landmarks,
    const std::array<int, 6>& indices) {
    std::array<Point2D, 6> ear{};
    for (size_t i = 0; i < 6; ++i) {
        ear[i] = landmarks[static_cast<size_t>(indices[i])];
    }
    return ear;
}

Result<cv::Mat> EyeExtractor::crop_eye(
    const cv::Mat& frame,
    const std::array<Point2D, 6>& ear,
    float padding) {
    if (frame.empty()) {
        return make_error(ErrorCode::InvalidInput, "Empty frame for eye crop");
    }

    // Compute bounding box from 6 EAR points
    float min_x = ear[0].x;
    float min_y = ear[0].y;
    float max_x = ear[0].x;
    float max_y = ear[0].y;
    for (size_t i = 1; i < 6; ++i) {
        min_x = std::min(min_x, ear[i].x);
        min_y = std::min(min_y, ear[i].y);
        max_x = std::max(max_x, ear[i].x);
        max_y = std::max(max_y, ear[i].y);
    }

    float w = max_x - min_x;
    float h = max_y - min_y;

    // Add padding
    float pad_x = w * padding;
    float pad_y = h * padding;
    min_x -= pad_x;
    min_y -= pad_y;
    max_x += pad_x;
    max_y += pad_y;

    // Clip to frame bounds
    int x1 = std::max(0, static_cast<int>(std::floor(min_x)));
    int y1 = std::max(0, static_cast<int>(std::floor(min_y)));
    int x2 = std::min(frame.cols, static_cast<int>(std::ceil(max_x)));
    int y2 = std::min(frame.rows, static_cast<int>(std::ceil(max_y)));

    if (x2 <= x1 || y2 <= y1) {
        return make_error(ErrorCode::EyeNotDetected,
                          "Eye crop region has zero area after clipping");
    }

    cv::Rect roi(x1, y1, x2 - x1, y2 - y1);
    return frame(roi).clone();
}

Result<EyeExtractionResult> EyeExtractor::run(
    const FaceROI& face, const cv::Mat& frame) const {
    if (frame.empty()) {
        return make_error(ErrorCode::InvalidInput, "Empty frame");
    }

    const auto& lm = face.landmarks;
    const auto num_landmarks = lm.size();

    // Determine which index set to use based on landmark count
    const std::array<int, 6>* left_indices = nullptr;
    const std::array<int, 6>* right_indices = nullptr;

    if (num_landmarks == kMediaPipeLandmarkCount) {
        left_indices = &kMediaPipeLeftEyeEAR;
        right_indices = &kMediaPipeRightEyeEAR;
    } else if (num_landmarks == kDlibLandmarkCount) {
        left_indices = &kDlibLeftEyeEAR;
        right_indices = &kDlibRightEyeEAR;
    } else {
        return make_error(ErrorCode::InvalidInput,
                          "Unsupported landmark count: " + std::to_string(num_landmarks),
                          "Expected 468 (MediaPipe) or 68 (dlib)");
    }

    auto left_ear = extract_ear(lm, *left_indices);
    auto right_ear = extract_ear(lm, *right_indices);

    auto left_crop = crop_eye(frame, left_ear, config_.padding);
    if (!left_crop) return std::unexpected(left_crop.error());

    auto right_crop = crop_eye(frame, right_ear, config_.padding);
    if (!right_crop) return std::unexpected(right_crop.error());

    EyeExtractionResult result;
    result.landmarks.left = left_ear;
    result.landmarks.right = right_ear;
    result.left_eye_crop = std::move(left_crop.value());
    result.right_eye_crop = std::move(right_crop.value());

    return result;
}

Result<void> EyeExtractor::execute(
    std::unordered_map<std::string, std::any>& context) const {
    // Get face ROI
    auto face_it = context.find("face_roi");
    if (face_it == context.end()) {
        return make_error(ErrorCode::InvalidInput,
                          "Missing 'face_roi' in pipeline context");
    }
    const auto& face = std::any_cast<const FaceROI&>(face_it->second);

    // Get frame
    cv::Mat frame;
    if (auto it = context.find("preprocessed_color"); it != context.end()) {
        frame = std::any_cast<const cv::Mat&>(it->second);
    } else if (auto it = context.find("frame"); it != context.end()) {
        frame = std::any_cast<const cv::Mat&>(it->second);
    } else {
        return make_error(ErrorCode::InvalidInput,
                          "Missing 'preprocessed_color' or 'frame' in pipeline context");
    }

    auto result = run(face, frame);
    if (!result) return std::unexpected(result.error());

    context["eye_landmarks"] = result->landmarks;
    context["left_eye_crop"] = std::move(result->left_eye_crop);
    context["right_eye_crop"] = std::move(result->right_eye_crop);
    return {};
}

const std::vector<std::string>& EyeExtractor::dependencies() const noexcept {
    return kDependencies;
}

EYETRACK_REGISTER_NODE("eyetrack.EyeExtractor", EyeExtractor);

}  // namespace eyetrack
