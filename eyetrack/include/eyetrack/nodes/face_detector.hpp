#pragma once

#include <any>
#include <array>
#include <memory>
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

// Forward-declare pimpl structs to keep header clean and type copyable.
struct FaceDetectorOnnx;
struct FaceDetectorDlib;

enum class FaceDetectorBackend : uint8_t {
    MediaPipe = 0,  // ONNX Runtime FaceMesh (468 landmarks)
    Dlib = 1,       // dlib HOG + shape predictor (68 landmarks)
};

/// Face detection with pluggable backends.
///
/// MediaPipe backend: ONNX model producing 468 landmarks.
/// dlib backend: HOG face detector + 68-landmark shape predictor.
///
/// Context keys:
///   Input:  "preprocessed_color" (cv::Mat, BGR, from Preprocessor)
///   Output: "face_roi" (FaceROI with bounding box, landmarks, confidence)
class FaceDetector : public Algorithm<FaceDetector>, public PipelineNodeBase {
public:
    static constexpr std::string_view kName = "eyetrack.FaceDetector";

    static constexpr int kModelInputSize = 192;
    static constexpr int kNumLandmarksMediaPipe = 468;
    static constexpr int kNumLandmarksDlib = 68;
    static constexpr int kLandmarkDims = 3;  // x, y, z
    static constexpr float kDefaultConfidenceThreshold = 0.5F;

    // dlib 68-landmark indices for 6-point EAR (left and right eye)
    static constexpr std::array<int, 6> kDlibLeftEyeEAR = {36, 37, 38, 39, 40, 41};
    static constexpr std::array<int, 6> kDlibRightEyeEAR = {42, 43, 44, 45, 46, 47};

    struct Config {
        FaceDetectorBackend backend = FaceDetectorBackend::MediaPipe;
        std::string model_path = "models/face_landmark.onnx";
        std::string dlib_shape_predictor_path = "models/shape_predictor_68_face_landmarks.dat";
        float confidence_threshold = kDefaultConfidenceThreshold;
    };

    FaceDetector();
    explicit FaceDetector(const Config& config);
    explicit FaceDetector(const NodeParams& params);

    /// Detect face landmarks in a BGR frame.
    [[nodiscard]] Result<FaceROI> run(const cv::Mat& input) const;

    /// Preprocess a BGR frame for the MediaPipe ONNX model.
    /// Output shape: [1, 192, 192, 3], values in [0, 1], RGB order.
    [[nodiscard]] static Result<cv::Mat> preprocess(const cv::Mat& input);

    /// Get the currently configured backend.
    [[nodiscard]] FaceDetectorBackend backend() const noexcept { return config_.backend; }

    /// Extract 6-point EAR subset from dlib 68-landmark set for a given eye.
    [[nodiscard]] static std::array<Point2D, 6> dlib_ear_subset(
        const std::vector<Point2D>& landmarks_68,
        const std::array<int, 6>& indices);

    // -- PipelineNodeBase interface --
    Result<void> execute(
        std::unordered_map<std::string, std::any>& context) const override;
    [[nodiscard]] std::string_view node_name() const noexcept override { return kName; }
    [[nodiscard]] const std::vector<std::string>& dependencies() const noexcept override;

private:
    Config config_;
    static const std::vector<std::string> kDependencies;

    // MediaPipe ONNX backend
#ifdef EYETRACK_HAS_ONNXRUNTIME
    mutable std::shared_ptr<FaceDetectorOnnx> onnx_;
    [[nodiscard]] Result<void> load_onnx_model() const;
    [[nodiscard]] Result<FaceROI> run_mediapipe(const cv::Mat& input) const;
#endif

    // dlib backend
#ifdef EYETRACK_HAS_DLIB
    mutable std::shared_ptr<FaceDetectorDlib> dlib_;
    [[nodiscard]] Result<void> load_dlib_model() const;
    [[nodiscard]] Result<FaceROI> run_dlib(const cv::Mat& input) const;
#endif
};

}  // namespace eyetrack
