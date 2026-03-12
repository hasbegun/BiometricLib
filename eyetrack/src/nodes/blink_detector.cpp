#include <eyetrack/nodes/blink_detector.hpp>

#include <eyetrack/core/node_registry.hpp>
#include <eyetrack/utils/geometry_utils.hpp>

namespace eyetrack {

const std::vector<std::string> BlinkDetector::kDependencies = {
    "eyetrack.EyeExtractor"};

BlinkDetector::BlinkDetector() = default;

BlinkDetector::BlinkDetector(const Config& config) : config_(config) {}

BlinkDetector::BlinkDetector(const NodeParams& params) {
    if (auto it = params.find("ear_threshold"); it != params.end())
        config_.ear_threshold = std::stof(it->second);
    if (auto it = params.find("min_blink_ms"); it != params.end())
        config_.min_blink_ms = std::stoi(it->second);
    if (auto it = params.find("max_single_ms"); it != params.end())
        config_.max_single_ms = std::stoi(it->second);
    if (auto it = params.find("long_threshold_ms"); it != params.end())
        config_.long_threshold_ms = std::stoi(it->second);
    if (auto it = params.find("double_window_ms"); it != params.end())
        config_.double_window_ms = std::stoi(it->second);
}

void BlinkDetector::set_baseline(float baseline_ear) {
    baseline_ear_ = baseline_ear;
    has_baseline_ = true;
}

float BlinkDetector::effective_threshold() const noexcept {
    if (has_baseline_) {
        // Threshold relative to baseline: typically 70% of open-eye EAR
        return baseline_ear_ * 0.7F;
    }
    return config_.ear_threshold;
}

void BlinkDetector::reset() {
    state_ = BlinkState::Open;
    closing_start_ = {};
    last_single_time_ = {};
}

BlinkType BlinkDetector::update(
    float ear,
    std::chrono::steady_clock::time_point timestamp) {

    float threshold = effective_threshold();

    switch (state_) {
    case BlinkState::Open:
        if (ear < threshold) {
            state_ = BlinkState::Closing;
            closing_start_ = timestamp;
        }
        return BlinkType::None;

    case BlinkState::Closing:
        if (ear >= threshold) {
            // Eyes reopened — compute how long they were closed
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                                timestamp - closing_start_)
                                .count();

            if (duration < config_.min_blink_ms) {
                // Too short — noise
                state_ = BlinkState::Open;
                return BlinkType::None;
            }

            if (duration >= config_.long_threshold_ms) {
                // Long blink
                state_ = BlinkState::Open;
                return BlinkType::Long;
            }

            if (duration <= config_.max_single_ms) {
                // Could be single or first of double
                state_ = BlinkState::WaitingDouble;
                last_single_time_ = timestamp;
                return BlinkType::None;  // Don't emit yet, wait for potential double
            }

            // Between max_single and long_threshold — treat as single
            state_ = BlinkState::Open;
            return BlinkType::Single;
        }
        // Still closing — check if it's been long enough to be a Long blink
        {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                                timestamp - closing_start_)
                                .count();
            if (duration >= config_.long_threshold_ms) {
                state_ = BlinkState::Open;
                return BlinkType::Long;
            }
        }
        return BlinkType::None;

    case BlinkState::WaitingDouble: {
        auto since_single = std::chrono::duration_cast<std::chrono::milliseconds>(
                                timestamp - last_single_time_)
                                .count();

        if (ear < threshold) {
            // Another blink starting — this could become a double
            state_ = BlinkState::Closing;
            closing_start_ = timestamp;

            if (since_single <= config_.double_window_ms) {
                // Within double window — upgrade to double when this blink ends
                // For simplicity, emit Double now and go back to Closing
                // (the closing state will handle the second blink's duration)
                return BlinkType::Double;
            }
            // Outside window — emit the pending Single, start new blink tracking
            return BlinkType::Single;
        }

        if (since_single > config_.double_window_ms) {
            // Timeout — confirm the pending Single
            state_ = BlinkState::Open;
            return BlinkType::Single;
        }

        return BlinkType::None;
    }
    }

    return BlinkType::None;
}

Result<void> BlinkDetector::execute(
    std::unordered_map<std::string, std::any>& context) const {
    // Look for eye landmarks to compute EAR
    auto it = context.find("eye_landmarks");
    if (it == context.end()) {
        return make_error(ErrorCode::InvalidInput,
                          "No eye_landmarks in context");
    }

    const auto& landmarks =
        std::any_cast<const EyeLandmarks&>(it->second);
    float ear = compute_ear_average(landmarks);

    // Note: execute() is const but blink detection requires mutable state.
    // In pipeline usage, the detector should be used via update() directly.
    // Store the EAR in context for downstream use.
    context["ear"] = ear;

    return {};
}

const std::vector<std::string>& BlinkDetector::dependencies() const noexcept {
    return kDependencies;
}

EYETRACK_REGISTER_NODE("eyetrack.BlinkDetector", BlinkDetector);

}  // namespace eyetrack
