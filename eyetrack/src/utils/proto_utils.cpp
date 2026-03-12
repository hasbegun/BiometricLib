#ifdef EYETRACK_HAS_GRPC

#include <eyetrack/utils/proto_utils.hpp>

#include <opencv2/imgcodecs.hpp>

namespace eyetrack {

// ---- Point2D ----

proto::Point2D to_proto(const Point2D& p) {
    proto::Point2D pb;
    pb.set_x(p.x);
    pb.set_y(p.y);
    return pb;
}

Point2D from_proto(const proto::Point2D& p) {
    return {p.x(), p.y()};
}

// ---- EyeLandmarks ----

proto::EyeLandmarks to_proto(const EyeLandmarks& lm) {
    proto::EyeLandmarks pb;
    for (const auto& pt : lm.left) {
        *pb.add_left() = to_proto(pt);
    }
    for (const auto& pt : lm.right) {
        *pb.add_right() = to_proto(pt);
    }
    return pb;
}

EyeLandmarks from_proto(const proto::EyeLandmarks& lm) {
    EyeLandmarks result;
    for (int i = 0; i < lm.left_size() && i < 6; ++i) {
        result.left[static_cast<size_t>(i)] = from_proto(lm.left(i));
    }
    for (int i = 0; i < lm.right_size() && i < 6; ++i) {
        result.right[static_cast<size_t>(i)] = from_proto(lm.right(i));
    }
    return result;
}

// ---- PupilInfo ----

proto::PupilInfo to_proto(const PupilInfo& pi) {
    proto::PupilInfo pb;
    *pb.mutable_center() = to_proto(pi.center);
    pb.set_radius(pi.radius);
    pb.set_confidence(pi.confidence);
    return pb;
}

PupilInfo from_proto(const proto::PupilInfo& pi) {
    return {
        .center = from_proto(pi.center()),
        .radius = pi.radius(),
        .confidence = pi.confidence(),
    };
}

// ---- BlinkType ----

proto::BlinkType to_proto(BlinkType bt) {
    switch (bt) {
        case BlinkType::None:   return proto::BLINK_NONE;
        case BlinkType::Single: return proto::BLINK_SINGLE;
        case BlinkType::Double: return proto::BLINK_DOUBLE;
        case BlinkType::Long:   return proto::BLINK_LONG;
    }
    return proto::BLINK_NONE;
}

BlinkType from_proto(proto::BlinkType bt) {
    switch (bt) {
        case proto::BLINK_NONE:   return BlinkType::None;
        case proto::BLINK_SINGLE: return BlinkType::Single;
        case proto::BLINK_DOUBLE: return BlinkType::Double;
        case proto::BLINK_LONG:   return BlinkType::Long;
        default:                  return BlinkType::None;
    }
}

// ---- FaceROI ----

proto::FaceROI to_proto(const FaceROI& roi) {
    proto::FaceROI pb;
    auto* bbox = pb.mutable_bounding_box();
    bbox->set_x(roi.bounding_box.x);
    bbox->set_y(roi.bounding_box.y);
    bbox->set_width(roi.bounding_box.width);
    bbox->set_height(roi.bounding_box.height);
    for (const auto& pt : roi.landmarks) {
        *pb.add_landmarks() = to_proto(pt);
    }
    pb.set_confidence(roi.confidence);
    return pb;
}

FaceROI from_proto(const proto::FaceROI& roi) {
    FaceROI result;
    if (roi.has_bounding_box()) {
        const auto& bb = roi.bounding_box();
        result.bounding_box = cv::Rect2f(bb.x(), bb.y(), bb.width(), bb.height());
    }
    result.landmarks.reserve(static_cast<size_t>(roi.landmarks_size()));
    for (const auto& pt : roi.landmarks()) {
        result.landmarks.push_back(from_proto(pt));
    }
    result.confidence = roi.confidence();
    return result;
}

// ---- FrameData ----

proto::FrameData to_proto(const FrameData& fd) {
    proto::FrameData pb;
    // Encode frame as PNG bytes
    if (!fd.frame.empty()) {
        std::vector<uint8_t> buf;
        cv::imencode(".png", fd.frame, buf);
        pb.set_frame(buf.data(), buf.size());
        pb.set_width(static_cast<uint32_t>(fd.frame.cols));
        pb.set_height(static_cast<uint32_t>(fd.frame.rows));
        pb.set_channels(static_cast<uint32_t>(fd.frame.channels()));
    }
    pb.set_frame_id(fd.frame_id);
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        fd.timestamp.time_since_epoch()).count();
    pb.set_timestamp_ns(ns);
    pb.set_client_id(fd.client_id);
    return pb;
}

FrameData from_proto(const proto::FrameData& fd) {
    FrameData result;
    if (!fd.frame().empty()) {
        std::vector<uint8_t> buf(fd.frame().begin(), fd.frame().end());
        result.frame = cv::imdecode(buf, cv::IMREAD_UNCHANGED);
    }
    result.frame_id = fd.frame_id();
    result.timestamp = std::chrono::steady_clock::time_point(
        std::chrono::nanoseconds(fd.timestamp_ns()));
    result.client_id = fd.client_id();
    return result;
}

// ---- GazeResult -> GazeEvent ----

proto::GazeEvent to_proto(const GazeResult& gr, const std::string& client_id,
                          uint64_t frame_id) {
    proto::GazeEvent pb;
    *pb.mutable_gaze_point() = to_proto(gr.gaze_point);
    pb.set_confidence(gr.confidence);
    pb.set_blink(to_proto(gr.blink));
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        gr.timestamp.time_since_epoch()).count();
    pb.set_timestamp_ns(ns);
    pb.set_client_id(client_id);
    pb.set_frame_id(frame_id);
    return pb;
}

GazeResult from_proto(const proto::GazeEvent& ge) {
    GazeResult result;
    if (ge.has_gaze_point()) {
        result.gaze_point = from_proto(ge.gaze_point());
    }
    result.confidence = ge.confidence();
    result.blink = from_proto(ge.blink());
    result.timestamp = std::chrono::steady_clock::time_point(
        std::chrono::nanoseconds(ge.timestamp_ns()));
    return result;
}

// ---- CalibrationProfile -> CalibrationRequest ----

proto::CalibrationRequest to_proto(const CalibrationProfile& cp) {
    proto::CalibrationRequest pb;
    pb.set_user_id(cp.user_id);
    for (size_t i = 0; i < cp.screen_points.size(); ++i) {
        auto* point = pb.add_points();
        *point->mutable_screen_point() = to_proto(cp.screen_points[i]);
        if (i < cp.pupil_observations.size()) {
            *point->mutable_pupil_observation() = to_proto(cp.pupil_observations[i]);
        }
    }
    return pb;
}

CalibrationProfile from_proto(const proto::CalibrationRequest& cr) {
    CalibrationProfile result;
    result.user_id = cr.user_id();
    for (const auto& pt : cr.points()) {
        if (pt.has_screen_point()) {
            result.screen_points.push_back(from_proto(pt.screen_point()));
        }
        if (pt.has_pupil_observation()) {
            result.pupil_observations.push_back(from_proto(pt.pupil_observation()));
        }
    }
    return result;
}

}  // namespace eyetrack

#endif  // EYETRACK_HAS_GRPC
