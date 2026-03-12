#include <eyetrack/nodes/preprocessor.hpp>

#include <opencv2/imgproc.hpp>

#include <eyetrack/core/node_registry.hpp>

namespace eyetrack {

const std::vector<std::string> Preprocessor::kDependencies = {};

Preprocessor::Preprocessor() = default;

Preprocessor::Preprocessor(const Config& config) : config_(config) {}

Preprocessor::Preprocessor(const NodeParams& params) {
    if (auto it = params.find("target_width"); it != params.end())
        config_.target_width = static_cast<uint32_t>(std::stoul(it->second));
    if (auto it = params.find("target_height"); it != params.end())
        config_.target_height = static_cast<uint32_t>(std::stoul(it->second));
    if (auto it = params.find("clahe_clip_limit"); it != params.end())
        config_.clahe_clip_limit = std::stod(it->second);
    if (auto it = params.find("clahe_grid_size"); it != params.end())
        config_.clahe_grid_size = std::stoi(it->second);
    if (auto it = params.find("enable_denoise"); it != params.end())
        config_.enable_denoise = (it->second == "true" || it->second == "1");
}

Result<cv::Mat> Preprocessor::run(const cv::Mat& input) const {
    if (input.empty()) {
        return make_error(ErrorCode::InvalidInput, "Empty input frame");
    }

    cv::Mat resized;
    if (input.cols != static_cast<int>(config_.target_width) ||
        input.rows != static_cast<int>(config_.target_height)) {
        cv::resize(input, resized,
                   cv::Size(static_cast<int>(config_.target_width),
                            static_cast<int>(config_.target_height)));
    } else {
        resized = input;
    }

    // Convert to grayscale
    cv::Mat gray;
    if (resized.channels() == 3) {
        cv::cvtColor(resized, gray, cv::COLOR_BGR2GRAY);
    } else if (resized.channels() == 1) {
        gray = resized;
    } else {
        return make_error(ErrorCode::InvalidInput, "Unsupported channel count");
    }

    // Apply CLAHE (Contrast Limited Adaptive Histogram Equalization)
    auto clahe = cv::createCLAHE(config_.clahe_clip_limit,
                                  cv::Size(config_.clahe_grid_size, config_.clahe_grid_size));
    cv::Mat equalized;
    clahe->apply(gray, equalized);

    // Optional bilateral denoise
    if (config_.enable_denoise) {
        cv::Mat denoised;
        cv::bilateralFilter(equalized, denoised, config_.denoise_diameter,
                            config_.denoise_sigma_color, config_.denoise_sigma_space);
        return denoised;
    }

    return equalized;
}

Result<void> Preprocessor::execute(
    std::unordered_map<std::string, std::any>& context) const {
    auto it = context.find("frame");
    if (it == context.end()) {
        return make_error(ErrorCode::InvalidInput, "Missing 'frame' in pipeline context");
    }

    const auto& frame = std::any_cast<const cv::Mat&>(it->second);

    auto result = run(frame);
    if (!result) {
        return std::unexpected(result.error());
    }

    // Also store the resized color frame (useful for face detector)
    cv::Mat resized_color;
    if (frame.cols != static_cast<int>(config_.target_width) ||
        frame.rows != static_cast<int>(config_.target_height)) {
        cv::resize(frame, resized_color,
                   cv::Size(static_cast<int>(config_.target_width),
                            static_cast<int>(config_.target_height)));
    } else {
        resized_color = frame;
    }

    context["preprocessed_frame"] = std::move(result.value());
    context["preprocessed_color"] = std::move(resized_color);
    return {};
}

const std::vector<std::string>& Preprocessor::dependencies() const noexcept {
    return kDependencies;
}

EYETRACK_REGISTER_NODE("eyetrack.Preprocessor", Preprocessor);

}  // namespace eyetrack
