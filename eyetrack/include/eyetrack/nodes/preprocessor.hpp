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

namespace eyetrack {

/// Frame preprocessing: resize, grayscale conversion, CLAHE, optional bilateral denoise.
///
/// Context keys:
///   Input:  "frame" (cv::Mat, BGR)
///   Output: "preprocessed_frame" (cv::Mat, grayscale, equalized)
///           "preprocessed_color" (cv::Mat, BGR, resized)
class Preprocessor : public Algorithm<Preprocessor>, public PipelineNodeBase {
public:
    static constexpr std::string_view kName = "eyetrack.Preprocessor";

    struct Config {
        uint32_t target_width = 640;
        uint32_t target_height = 480;
        double clahe_clip_limit = 2.0;
        int clahe_grid_size = 8;
        bool enable_denoise = false;
        int denoise_diameter = 5;
        double denoise_sigma_color = 50.0;
        double denoise_sigma_space = 50.0;
    };

    Preprocessor();
    explicit Preprocessor(const Config& config);
    explicit Preprocessor(const NodeParams& params);

    /// Preprocess a single frame: resize + grayscale + CLAHE + optional denoise.
    [[nodiscard]] Result<cv::Mat> run(const cv::Mat& input) const;

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
