#pragma once

#include <any>
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

enum class PupilDetectorMethod : uint8_t {
    Centroid = 0,  // Adaptive threshold + contour centroid
    Ellipse = 1,   // Adaptive threshold + ellipse fitting
};

/// Detects pupil position in a cropped eye image.
///
/// Centroid method: adaptive threshold -> morphological close ->
///   findContours -> largest contour -> moments centroid + minEnclosingCircle.
/// Ellipse method: adaptive threshold -> contour -> fitEllipse -> center + axes.
///
/// Context keys:
///   Input:  "left_eye_crop" (cv::Mat from EyeExtractor)
///           "right_eye_crop" (cv::Mat from EyeExtractor)
///   Output: "left_pupil" (PupilInfo)
///           "right_pupil" (PupilInfo)
class PupilDetector : public Algorithm<PupilDetector>, public PipelineNodeBase {
public:
    static constexpr std::string_view kName = "eyetrack.PupilDetector";

    struct Config {
        PupilDetectorMethod method = PupilDetectorMethod::Centroid;
        int adaptive_block_size = 11;
        double adaptive_c = 3.0;
        int morph_kernel_size = 3;
        int min_contour_area = 10;
    };

    PupilDetector();
    explicit PupilDetector(const Config& config);
    explicit PupilDetector(const NodeParams& params);

    /// Detect pupil in a single cropped eye image.
    [[nodiscard]] Result<PupilInfo> run(const cv::Mat& eye_crop) const;

    /// Get current detection method.
    [[nodiscard]] PupilDetectorMethod method() const noexcept { return config_.method; }

    // -- PipelineNodeBase interface --
    Result<void> execute(
        std::unordered_map<std::string, std::any>& context) const override;
    [[nodiscard]] std::string_view node_name() const noexcept override { return kName; }
    [[nodiscard]] const std::vector<std::string>& dependencies() const noexcept override;

private:
    Config config_;
    static const std::vector<std::string> kDependencies;

    [[nodiscard]] Result<PupilInfo> run_centroid(const cv::Mat& gray) const;
    [[nodiscard]] Result<PupilInfo> run_ellipse(const cv::Mat& gray) const;
};

}  // namespace eyetrack
