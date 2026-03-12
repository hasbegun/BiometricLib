#include <eyetrack/pipeline/pipeline_context.hpp>

namespace eyetrack {

template <typename T>
std::optional<T> PipelineContext::get(const std::string& key) const {
    auto it = context_.find(key);
    if (it == context_.end()) return std::nullopt;
    try {
        return std::any_cast<T>(it->second);
    } catch (const std::bad_any_cast&) {
        return std::nullopt;
    }
}

void PipelineContext::set_frame_data(FrameData data) {
    context_["frame"] = data.frame;
    context_["frame_data"] = std::move(data);
}

std::optional<FrameData> PipelineContext::frame_data() const {
    return get<FrameData>("frame_data");
}

void PipelineContext::set_face_roi(FaceROI roi) {
    context_["face_roi"] = std::move(roi);
}

std::optional<FaceROI> PipelineContext::face_roi() const {
    return get<FaceROI>("face_roi");
}

void PipelineContext::set_eye_landmarks(EyeLandmarks landmarks) {
    context_["eye_landmarks"] = std::move(landmarks);
}

std::optional<EyeLandmarks> PipelineContext::eye_landmarks() const {
    return get<EyeLandmarks>("eye_landmarks");
}

void PipelineContext::set_left_pupil(PupilInfo pupil) {
    context_["left_pupil"] = pupil;
}

void PipelineContext::set_right_pupil(PupilInfo pupil) {
    context_["right_pupil"] = pupil;
}

std::optional<PupilInfo> PipelineContext::left_pupil() const {
    return get<PupilInfo>("left_pupil");
}

std::optional<PupilInfo> PipelineContext::right_pupil() const {
    return get<PupilInfo>("right_pupil");
}

void PipelineContext::set_gaze_result(GazeResult result) {
    context_["gaze_result"] = std::move(result);
}

std::optional<GazeResult> PipelineContext::gaze_result() const {
    return get<GazeResult>("gaze_result");
}

void PipelineContext::set_ear(float ear) {
    context_["ear"] = ear;
}

std::optional<float> PipelineContext::ear() const {
    return get<float>("ear");
}

void PipelineContext::set_blink_type(BlinkType blink) {
    context_["blink_type"] = blink;
}

std::optional<BlinkType> PipelineContext::blink_type() const {
    return get<BlinkType>("blink_type");
}

void PipelineContext::clear() {
    context_.clear();
}

}  // namespace eyetrack
