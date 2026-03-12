#include <eyetrack/nodes/gaze_estimator.hpp>

#include <algorithm>
#include <chrono>

#include <eyetrack/core/node_registry.hpp>
#include <eyetrack/utils/geometry_utils.hpp>

namespace eyetrack {

const std::vector<std::string> GazeEstimator::kDependencies = {
    "eyetrack.PupilDetector", "eyetrack.CalibrationManager"};

GazeEstimator::GazeEstimator() = default;

GazeEstimator::GazeEstimator(const NodeParams& /*params*/) {}

void GazeEstimator::set_calibration(const CalibrationTransform& transform) {
    transform_ = transform;
    calibrated_ = true;
}

Result<GazeResult> GazeEstimator::estimate(const PupilInfo& pupil) const {
    if (!calibrated_) {
        return make_error(ErrorCode::CalibrationNotLoaded,
                          "Gaze estimator has no calibration loaded");
    }

    // Apply polynomial mapping using left-eye transform
    auto gaze_point = apply_polynomial_mapping(pupil.center,
                                                transform_.transform_left);

    // Clamp to normalized [0, 1] range
    gaze_point.x = std::clamp(gaze_point.x, 0.0F, 1.0F);
    gaze_point.y = std::clamp(gaze_point.y, 0.0F, 1.0F);

    GazeResult result;
    result.gaze_point = gaze_point;
    result.confidence = pupil.confidence;
    result.timestamp = std::chrono::steady_clock::now();
    return result;
}

Result<void> GazeEstimator::execute(
    std::unordered_map<std::string, std::any>& context) const {
    // Get calibration transform from context if not already set
    const CalibrationTransform* transform = &transform_;
    CalibrationTransform ctx_transform;
    if (!calibrated_) {
        auto it = context.find("calibration_transform");
        if (it == context.end()) {
            return make_error(ErrorCode::CalibrationNotLoaded,
                              "No calibration transform in context");
        }
        ctx_transform =
            std::any_cast<const CalibrationTransform&>(it->second);
        transform = &ctx_transform;
    }

    // Try left pupil first, then right
    const PupilInfo* pupil = nullptr;
    if (auto it = context.find("left_pupil"); it != context.end()) {
        pupil = &std::any_cast<const PupilInfo&>(it->second);
    } else if (auto it2 = context.find("right_pupil"); it2 != context.end()) {
        pupil = &std::any_cast<const PupilInfo&>(it2->second);
    }

    if (pupil == nullptr) {
        return make_error(ErrorCode::PupilNotDetected,
                          "No pupil data in context");
    }

    auto gaze_point = apply_polynomial_mapping(pupil->center,
                                                transform->transform_left);
    gaze_point.x = std::clamp(gaze_point.x, 0.0F, 1.0F);
    gaze_point.y = std::clamp(gaze_point.y, 0.0F, 1.0F);

    GazeResult gaze;
    gaze.gaze_point = gaze_point;
    gaze.confidence = pupil->confidence;
    gaze.timestamp = std::chrono::steady_clock::now();
    context["gaze_result"] = gaze;

    return {};
}

const std::vector<std::string>& GazeEstimator::dependencies() const noexcept {
    return kDependencies;
}

EYETRACK_REGISTER_NODE("eyetrack.GazeEstimator", GazeEstimator);

}  // namespace eyetrack
