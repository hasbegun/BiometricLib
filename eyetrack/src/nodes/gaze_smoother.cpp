#include <eyetrack/nodes/gaze_smoother.hpp>

#include <eyetrack/core/node_registry.hpp>

namespace eyetrack {

const std::vector<std::string> GazeSmoother::kDependencies = {
    "eyetrack.GazeEstimator"};

GazeSmoother::GazeSmoother() = default;

GazeSmoother::GazeSmoother(const Config& config) : config_(config) {}

GazeSmoother::GazeSmoother(const NodeParams& params) {
    if (auto it = params.find("alpha"); it != params.end()) {
        config_.alpha = std::stof(it->second);
    }
}

GazeResult GazeSmoother::smooth(const GazeResult& current) {
    if (!has_previous_) {
        has_previous_ = true;
        previous_ = current;
        return current;
    }

    GazeResult smoothed = current;
    smoothed.gaze_point.x = config_.alpha * current.gaze_point.x +
                            (1.0F - config_.alpha) * previous_.gaze_point.x;
    smoothed.gaze_point.y = config_.alpha * current.gaze_point.y +
                            (1.0F - config_.alpha) * previous_.gaze_point.y;

    previous_ = smoothed;
    return smoothed;
}

void GazeSmoother::reset() {
    has_previous_ = false;
    previous_ = GazeResult{};
}

Result<void> GazeSmoother::execute(
    std::unordered_map<std::string, std::any>& context) const {
    auto it = context.find("gaze_result");
    if (it == context.end()) {
        return make_error(ErrorCode::InvalidInput,
                          "No gaze_result in context");
    }

    // Note: execute() is const but smoothing requires mutable state.
    // In pipeline usage, the smoother should be used via smooth() directly.
    // For the pipeline node interface, we pass through without smoothing.
    return {};
}

const std::vector<std::string>& GazeSmoother::dependencies() const noexcept {
    return kDependencies;
}

EYETRACK_REGISTER_NODE("eyetrack.GazeSmoother", GazeSmoother);

}  // namespace eyetrack
