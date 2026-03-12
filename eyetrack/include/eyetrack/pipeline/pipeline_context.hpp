#pragma once

#include <any>
#include <optional>
#include <string>
#include <unordered_map>

#include <eyetrack/core/types.hpp>

namespace eyetrack {

/// Typed wrapper around the per-frame shared context map.
///
/// Provides type-safe accessors for all pipeline data types while
/// remaining compatible with the raw context map used by PipelineNodeBase.
class PipelineContext {
public:
    /// Set/get FrameData.
    void set_frame_data(FrameData data);
    [[nodiscard]] std::optional<FrameData> frame_data() const;

    /// Set/get FaceROI.
    void set_face_roi(FaceROI roi);
    [[nodiscard]] std::optional<FaceROI> face_roi() const;

    /// Set/get EyeLandmarks.
    void set_eye_landmarks(EyeLandmarks landmarks);
    [[nodiscard]] std::optional<EyeLandmarks> eye_landmarks() const;

    /// Set/get PupilInfo (left and right).
    void set_left_pupil(PupilInfo pupil);
    void set_right_pupil(PupilInfo pupil);
    [[nodiscard]] std::optional<PupilInfo> left_pupil() const;
    [[nodiscard]] std::optional<PupilInfo> right_pupil() const;

    /// Set/get GazeResult.
    void set_gaze_result(GazeResult result);
    [[nodiscard]] std::optional<GazeResult> gaze_result() const;

    /// Set/get EAR value.
    void set_ear(float ear);
    [[nodiscard]] std::optional<float> ear() const;

    /// Set/get BlinkType.
    void set_blink_type(BlinkType blink);
    [[nodiscard]] std::optional<BlinkType> blink_type() const;

    /// Clear all fields.
    void clear();

    /// Access the raw context map (for PipelineNodeBase::execute).
    [[nodiscard]] std::unordered_map<std::string, std::any>& raw() noexcept {
        return context_;
    }
    [[nodiscard]] const std::unordered_map<std::string, std::any>& raw()
        const noexcept {
        return context_;
    }

private:
    std::unordered_map<std::string, std::any> context_;

    template <typename T>
    std::optional<T> get(const std::string& key) const;
};

}  // namespace eyetrack
