#include <eyetrack/core/types.hpp>

namespace eyetrack {

std::ostream& operator<<(std::ostream& os, const Point2D& p) {
    os << "Point2D(" << p.x << ", " << p.y << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const EyeLandmarks& lm) {
    os << "EyeLandmarks{left=[";
    for (size_t i = 0; i < lm.left.size(); ++i) {
        if (i > 0) os << ", ";
        os << lm.left[i];
    }
    os << "], right=[";
    for (size_t i = 0; i < lm.right.size(); ++i) {
        if (i > 0) os << ", ";
        os << lm.right[i];
    }
    os << "]}";
    return os;
}

std::ostream& operator<<(std::ostream& os, const PupilInfo& pi) {
    os << "PupilInfo{center=" << pi.center
       << ", radius=" << pi.radius
       << ", confidence=" << pi.confidence << "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, BlinkType bt) {
    switch (bt) {
        case BlinkType::None:   os << "None"; break;
        case BlinkType::Single: os << "Single"; break;
        case BlinkType::Double: os << "Double"; break;
        case BlinkType::Long:   os << "Long"; break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const GazeResult& gr) {
    os << "GazeResult{gaze=" << gr.gaze_point
       << ", confidence=" << gr.confidence
       << ", blink=" << gr.blink << "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, const FaceROI& roi) {
    os << "FaceROI{bbox=["
       << roi.bounding_box.x << "," << roi.bounding_box.y << " "
       << roi.bounding_box.width << "x" << roi.bounding_box.height
       << "], landmarks=" << roi.landmarks.size()
       << ", confidence=" << roi.confidence << "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, const FrameData& fd) {
    os << "FrameData{id=" << fd.frame_id
       << ", size=" << fd.frame.cols << "x" << fd.frame.rows
       << ", client=" << fd.client_id << "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, const CalibrationProfile& cp) {
    os << "CalibrationProfile{user=" << cp.user_id
       << ", points=" << cp.screen_points.size()
       << ", observations=" << cp.pupil_observations.size() << "}";
    return os;
}

}  // namespace eyetrack
