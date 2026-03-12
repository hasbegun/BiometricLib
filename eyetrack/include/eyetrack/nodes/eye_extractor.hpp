#pragma once

#include <any>
#include <array>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <opencv2/core.hpp>

#include <eyetrack/core/algorithm.hpp>
#include <eyetrack/core/error.hpp>
#include <eyetrack/core/node_registry.hpp>
#include <eyetrack/core/pipeline_node.hpp>
#include <eyetrack/core/types.hpp>

namespace eyetrack {

/// Output from eye extraction: EAR landmarks + cropped eye images.
struct EyeExtractionResult {
    EyeLandmarks landmarks;
    cv::Mat left_eye_crop;
    cv::Mat right_eye_crop;
};

/// Extracts per-eye EAR landmarks and cropped eye regions from face landmarks.
///
/// Supports both MediaPipe 468-landmark and dlib 68-landmark inputs.
/// For MediaPipe, extracts 6-point EAR subsets using FaceMesh indices.
/// For dlib, extracts using dlib's eye landmark indices.
///
/// Context keys:
///   Input:  "face_roi" (FaceROI from FaceDetector)
///           "preprocessed_color" or "frame" (cv::Mat, BGR)
///   Output: "eye_landmarks" (EyeLandmarks)
///           "left_eye_crop" (cv::Mat)
///           "right_eye_crop" (cv::Mat)
class EyeExtractor : public Algorithm<EyeExtractor>, public PipelineNodeBase {
public:
    static constexpr std::string_view kName = "eyetrack.EyeExtractor";

    // MediaPipe FaceMesh 468 landmark indices for 6-point EAR
    static constexpr std::array<int, 6> kMediaPipeLeftEyeEAR = {362, 385, 387, 263, 373, 380};
    static constexpr std::array<int, 6> kMediaPipeRightEyeEAR = {33, 160, 158, 133, 153, 144};

    // dlib 68-landmark indices for 6-point EAR
    static constexpr std::array<int, 6> kDlibLeftEyeEAR = {36, 37, 38, 39, 40, 41};
    static constexpr std::array<int, 6> kDlibRightEyeEAR = {42, 43, 44, 45, 46, 47};

    static constexpr int kMediaPipeLandmarkCount = 468;
    static constexpr int kDlibLandmarkCount = 68;

    struct Config {
        float padding = 0.2F;  // 20% padding around eye bounding box
    };

    EyeExtractor();
    explicit EyeExtractor(const Config& config);
    explicit EyeExtractor(const NodeParams& params);

    /// Extract eye landmarks and crops from a FaceROI and source frame.
    [[nodiscard]] Result<EyeExtractionResult> run(
        const FaceROI& face, const cv::Mat& frame) const;

    /// Extract 6-point EAR subset from a landmark vector using given indices.
    [[nodiscard]] static std::array<Point2D, 6> extract_ear(
        const std::vector<Point2D>& landmarks,
        const std::array<int, 6>& indices);

    /// Crop an eye region from a frame given 6 EAR landmarks + padding.
    [[nodiscard]] static Result<cv::Mat> crop_eye(
        const cv::Mat& frame,
        const std::array<Point2D, 6>& ear,
        float padding);

    // -- PipelineNodeBase interface --
    Result<void> execute(
        std::unordered_map<std::string, std::any>& context) const override;
    [[nodiscard]] std::string_view node_name() const noexcept override { return kName; }
    [[nodiscard]] const std::vector<std::string>& dependencies() const noexcept override;

private:
    Config config_;
    static const std::vector<std::string> kDependencies;
};

}  // namespace eyetrack
