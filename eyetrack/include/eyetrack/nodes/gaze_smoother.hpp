#pragma once

#include <any>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <eyetrack/core/algorithm.hpp>
#include <eyetrack/core/error.hpp>
#include <eyetrack/core/node_registry.hpp>
#include <eyetrack/core/pipeline_node.hpp>
#include <eyetrack/core/types.hpp>

namespace eyetrack {

/// Smooths gaze output using exponential moving average (EMA).
///
/// smoothed = alpha * current + (1 - alpha) * previous
///
/// Context keys:
///   Input:  "gaze_result" (GazeResult)
///   Output: "gaze_result" (GazeResult, overwritten with smoothed value)
class GazeSmoother : public Algorithm<GazeSmoother>, public PipelineNodeBase {
public:
    static constexpr std::string_view kName = "eyetrack.GazeSmoother";

    struct Config {
        float alpha = 0.3F;  // EMA smoothing factor (0 = no update, 1 = no smoothing)
    };

    GazeSmoother();
    explicit GazeSmoother(const Config& config);
    explicit GazeSmoother(const NodeParams& params);

    /// Smooth a single gaze point. Maintains internal state.
    [[nodiscard]] GazeResult smooth(const GazeResult& current);

    /// Reset smoothing state (e.g., after recalibration).
    void reset() override;

    /// Get current alpha.
    [[nodiscard]] float alpha() const noexcept { return config_.alpha; }

    // -- PipelineNodeBase interface --
    Result<void> execute(
        std::unordered_map<std::string, std::any>& context) const override;
    [[nodiscard]] std::string_view node_name() const noexcept override {
        return kName;
    }
    [[nodiscard]] const std::vector<std::string>& dependencies()
        const noexcept override;

private:
    Config config_;
    bool has_previous_ = false;
    GazeResult previous_{};

    static const std::vector<std::string> kDependencies;
};

}  // namespace eyetrack
