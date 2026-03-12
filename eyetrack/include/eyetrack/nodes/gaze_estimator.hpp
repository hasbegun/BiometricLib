#pragma once

#include <any>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <Eigen/Dense>

#include <eyetrack/core/algorithm.hpp>
#include <eyetrack/core/error.hpp>
#include <eyetrack/core/node_registry.hpp>
#include <eyetrack/core/pipeline_node.hpp>
#include <eyetrack/core/types.hpp>
#include <eyetrack/utils/calibration_utils.hpp>

namespace eyetrack {

/// Estimates gaze screen coordinate from pupil position using calibrated
/// polynomial regression.
///
/// Context keys:
///   Input:  "left_pupil" / "right_pupil" (PupilInfo)
///           "calibration_transform" (CalibrationTransform)
///   Output: "gaze_result" (GazeResult)
class GazeEstimator : public Algorithm<GazeEstimator>, public PipelineNodeBase {
public:
    static constexpr std::string_view kName = "eyetrack.GazeEstimator";

    GazeEstimator();
    explicit GazeEstimator(const NodeParams& params);

    /// Set calibration coefficients (must be called before estimate()).
    void set_calibration(const CalibrationTransform& transform);

    /// Check if calibration is loaded.
    [[nodiscard]] bool is_calibrated() const noexcept { return calibrated_; }

    /// Estimate gaze from a single pupil observation.
    [[nodiscard]] Result<GazeResult> estimate(const PupilInfo& pupil) const;

    // -- PipelineNodeBase interface --
    Result<void> execute(
        std::unordered_map<std::string, std::any>& context) const override;
    [[nodiscard]] std::string_view node_name() const noexcept override {
        return kName;
    }
    [[nodiscard]] const std::vector<std::string>& dependencies()
        const noexcept override;

private:
    CalibrationTransform transform_;
    bool calibrated_ = false;

    static const std::vector<std::string> kDependencies;
};

}  // namespace eyetrack
