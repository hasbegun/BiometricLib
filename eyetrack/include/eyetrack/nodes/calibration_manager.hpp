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
#include <eyetrack/utils/calibration_utils.hpp>

namespace eyetrack {

/// Manages calibration sessions: collect points, fit coefficients, save/load profiles.
///
/// Context keys:
///   Input:  "calibration_command" (std::string: "start" | "add_point" | "finish" | "load")
///           "screen_point" (Point2D, for add_point)
///           "left_pupil" / "right_pupil" (PupilInfo, for add_point)
///           "calibration_path" (std::string, for save/load)
///           "user_id" (std::string, for get_profile)
///   Output: "calibration_transform" (CalibrationTransform)
///           "calibration_profile" (CalibrationProfile)
class CalibrationManager : public Algorithm<CalibrationManager>,
                           public PipelineNodeBase {
public:
    static constexpr std::string_view kName = "eyetrack.CalibrationManager";

    CalibrationManager();
    explicit CalibrationManager(const NodeParams& params);

    /// Start a new calibration session, clearing any previous state.
    void start_calibration(const std::string& user_id);

    /// Add a calibration point (screen target + observed pupil).
    void add_point(Point2D screen_point, PupilInfo pupil);

    /// Number of collected calibration points.
    [[nodiscard]] size_t point_count() const noexcept;

    /// Finish calibration: fit polynomial coefficients from collected points.
    [[nodiscard]] Result<CalibrationTransform> finish_calibration();

    /// Save current profile + transform to a YAML file.
    [[nodiscard]] Result<void> save_profile(const std::string& path) const;

    /// Load a profile + transform from a YAML file.
    [[nodiscard]] Result<void> load_profile(const std::string& path);

    /// Get stored profile by user_id.
    [[nodiscard]] Result<CalibrationProfile> get_profile(
        const std::string& user_id) const;

    /// Get the current transform (after finish or load).
    [[nodiscard]] const CalibrationTransform& transform() const noexcept {
        return transform_;
    }

    /// Get the current profile.
    [[nodiscard]] const CalibrationProfile& profile() const noexcept {
        return profile_;
    }

    // -- PipelineNodeBase interface --
    Result<void> execute(
        std::unordered_map<std::string, std::any>& context) const override;
    [[nodiscard]] std::string_view node_name() const noexcept override {
        return kName;
    }
    [[nodiscard]] const std::vector<std::string>& dependencies()
        const noexcept override;

private:
    CalibrationProfile profile_;
    CalibrationTransform transform_;
    bool calibrated_ = false;

    // Stored profiles by user_id (in-memory cache)
    std::unordered_map<std::string, CalibrationProfile> profiles_;
    std::unordered_map<std::string, CalibrationTransform> transforms_;

    static const std::vector<std::string> kDependencies;
};

}  // namespace eyetrack
