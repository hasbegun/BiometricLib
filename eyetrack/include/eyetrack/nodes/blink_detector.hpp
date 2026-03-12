#pragma once

#include <any>
#include <chrono>
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

/// Internal state of the blink detection state machine.
enum class BlinkState : uint8_t {
    Open = 0,           // Eyes open, normal EAR
    Closing = 1,        // EAR dropped below threshold, timing started
    WaitingDouble = 2,  // Single blink detected, waiting for potential second
};

/// Detects blinks from an EAR (Eye Aspect Ratio) stream using a
/// temporal state machine.
///
/// State transitions:
///   OPEN + EAR < threshold        -> CLOSING (record start_time)
///   CLOSING + EAR >= threshold    -> compute duration:
///     <min_blink_ms               -> noise, back to OPEN
///     min_blink_ms..max_single_ms -> Single, enter WAITING_DOUBLE
///     >=long_threshold_ms         -> Long, back to OPEN
///   WAITING_DOUBLE + another Single within double_window_ms -> Double, OPEN
///   WAITING_DOUBLE + timeout      -> confirm Single, back to OPEN
///
/// Context keys:
///   Input:  "left_pupil" / "right_pupil" (PupilInfo — not used directly)
///           "eye_landmarks" (EyeLandmarks — used to compute EAR)
///   Output: "blink_type" (BlinkType)
class BlinkDetector : public Algorithm<BlinkDetector>, public PipelineNodeBase {
public:
    static constexpr std::string_view kName = "eyetrack.BlinkDetector";

    struct Config {
        float ear_threshold = 0.21F;        // EAR below this = eyes closing
        int min_blink_ms = 100;             // Shorter = noise
        int max_single_ms = 400;            // Single blink upper bound
        int long_threshold_ms = 500;        // >= this = Long blink
        int double_window_ms = 500;         // Window for second blink -> Double
    };

    BlinkDetector();
    explicit BlinkDetector(const Config& config);
    explicit BlinkDetector(const NodeParams& params);

    /// Feed a single EAR value with its timestamp. Returns the detected blink
    /// type (BlinkType::None if no event on this frame).
    [[nodiscard]] BlinkType update(
        float ear,
        std::chrono::steady_clock::time_point timestamp);

    /// Set per-user EAR baseline. Threshold is adjusted relative to this.
    void set_baseline(float baseline_ear);

    /// Get current state (for testing).
    [[nodiscard]] BlinkState state() const noexcept { return state_; }

    /// Get effective threshold (base threshold or adjusted by baseline).
    [[nodiscard]] float effective_threshold() const noexcept;

    /// Get current config.
    [[nodiscard]] const Config& config() const noexcept { return config_; }

    /// Reset state machine to OPEN.
    void reset() override;

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
    BlinkState state_ = BlinkState::Open;
    std::chrono::steady_clock::time_point closing_start_{};
    std::chrono::steady_clock::time_point last_single_time_{};
    float baseline_ear_ = 0.0F;
    bool has_baseline_ = false;

    static const std::vector<std::string> kDependencies;
};

}  // namespace eyetrack
