#pragma once

#ifdef EYETRACK_HAS_GRPC

#include <eyetrack/core/types.hpp>
#include <eyetrack.pb.h>

namespace eyetrack {

// ---- Point2D ----

proto::Point2D to_proto(const Point2D& p);
Point2D from_proto(const proto::Point2D& p);

// ---- EyeLandmarks ----

proto::EyeLandmarks to_proto(const EyeLandmarks& lm);
EyeLandmarks from_proto(const proto::EyeLandmarks& lm);

// ---- PupilInfo ----

proto::PupilInfo to_proto(const PupilInfo& pi);
PupilInfo from_proto(const proto::PupilInfo& pi);

// ---- BlinkType ----

proto::BlinkType to_proto(BlinkType bt);
BlinkType from_proto(proto::BlinkType bt);

// ---- FaceROI ----

proto::FaceROI to_proto(const FaceROI& roi);
FaceROI from_proto(const proto::FaceROI& roi);

// ---- FrameData ----

proto::FrameData to_proto(const FrameData& fd);
FrameData from_proto(const proto::FrameData& fd);

// ---- GazeResult -> GazeEvent ----

proto::GazeEvent to_proto(const GazeResult& gr, const std::string& client_id = {},
                          uint64_t frame_id = 0);
GazeResult from_proto(const proto::GazeEvent& ge);

// ---- CalibrationProfile -> CalibrationRequest ----

proto::CalibrationRequest to_proto(const CalibrationProfile& cp);
CalibrationProfile from_proto(const proto::CalibrationRequest& cr);

}  // namespace eyetrack

#endif  // EYETRACK_HAS_GRPC
