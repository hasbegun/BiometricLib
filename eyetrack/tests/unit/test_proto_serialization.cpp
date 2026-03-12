#ifdef EYETRACK_HAS_GRPC

#include <gtest/gtest.h>

#include <eyetrack/utils/proto_utils.hpp>

namespace {

using namespace eyetrack;

// ---- Point2D ----

TEST(ProtoRoundTrip, point2d) {
    Point2D original{1.5F, 2.5F};
    auto pb = to_proto(original);
    auto restored = from_proto(pb);
    EXPECT_FLOAT_EQ(restored.x, original.x);
    EXPECT_FLOAT_EQ(restored.y, original.y);
}

// ---- EyeLandmarks ----

TEST(ProtoRoundTrip, eye_landmarks) {
    EyeLandmarks original;
    for (size_t i = 0; i < 6; ++i) {
        original.left[i] = {static_cast<float>(i), static_cast<float>(i + 10)};
        original.right[i] = {static_cast<float>(i + 20), static_cast<float>(i + 30)};
    }

    auto pb = to_proto(original);
    auto restored = from_proto(pb);

    for (size_t i = 0; i < 6; ++i) {
        EXPECT_FLOAT_EQ(restored.left[i].x, original.left[i].x);
        EXPECT_FLOAT_EQ(restored.left[i].y, original.left[i].y);
        EXPECT_FLOAT_EQ(restored.right[i].x, original.right[i].x);
        EXPECT_FLOAT_EQ(restored.right[i].y, original.right[i].y);
    }
}

// ---- PupilInfo ----

TEST(ProtoRoundTrip, pupil_info) {
    PupilInfo original{.center = {100.5F, 200.3F}, .radius = 15.7F, .confidence = 0.95F};
    auto pb = to_proto(original);
    auto restored = from_proto(pb);
    EXPECT_FLOAT_EQ(restored.center.x, original.center.x);
    EXPECT_FLOAT_EQ(restored.center.y, original.center.y);
    EXPECT_FLOAT_EQ(restored.radius, original.radius);
    EXPECT_FLOAT_EQ(restored.confidence, original.confidence);
}

// ---- GazeResult -> GazeEvent ----

TEST(ProtoRoundTrip, gaze_result) {
    auto now = std::chrono::steady_clock::time_point(std::chrono::nanoseconds(123456789));
    GazeResult original{
        .gaze_point = {0.5F, 0.7F},
        .confidence = 0.85F,
        .blink = BlinkType::None,
        .timestamp = now,
    };

    auto pb = to_proto(original, "client-1", 42);
    auto restored = from_proto(pb);

    EXPECT_FLOAT_EQ(restored.gaze_point.x, original.gaze_point.x);
    EXPECT_FLOAT_EQ(restored.gaze_point.y, original.gaze_point.y);
    EXPECT_FLOAT_EQ(restored.confidence, original.confidence);
    EXPECT_EQ(restored.blink, original.blink);
    EXPECT_EQ(restored.timestamp, original.timestamp);
}

// ---- BlinkType ----

TEST(ProtoRoundTrip, blink_type) {
    EXPECT_EQ(from_proto(to_proto(BlinkType::None)), BlinkType::None);
    EXPECT_EQ(from_proto(to_proto(BlinkType::Single)), BlinkType::Single);
    EXPECT_EQ(from_proto(to_proto(BlinkType::Double)), BlinkType::Double);
    EXPECT_EQ(from_proto(to_proto(BlinkType::Long)), BlinkType::Long);
}

// ---- FaceROI ----

TEST(ProtoRoundTrip, face_roi) {
    FaceROI original;
    original.bounding_box = cv::Rect2f(10.0F, 20.0F, 100.0F, 120.0F);
    original.confidence = 0.99F;
    original.landmarks = {{0.1F, 0.2F}, {0.3F, 0.4F}, {0.5F, 0.6F}};

    auto pb = to_proto(original);
    auto restored = from_proto(pb);

    EXPECT_FLOAT_EQ(restored.bounding_box.x, original.bounding_box.x);
    EXPECT_FLOAT_EQ(restored.bounding_box.y, original.bounding_box.y);
    EXPECT_FLOAT_EQ(restored.bounding_box.width, original.bounding_box.width);
    EXPECT_FLOAT_EQ(restored.bounding_box.height, original.bounding_box.height);
    EXPECT_FLOAT_EQ(restored.confidence, original.confidence);
    ASSERT_EQ(restored.landmarks.size(), original.landmarks.size());
    for (size_t i = 0; i < original.landmarks.size(); ++i) {
        EXPECT_FLOAT_EQ(restored.landmarks[i].x, original.landmarks[i].x);
        EXPECT_FLOAT_EQ(restored.landmarks[i].y, original.landmarks[i].y);
    }
}

// ---- FrameData ----

TEST(ProtoRoundTrip, frame_data) {
    FrameData original;
    original.frame = cv::Mat::zeros(48, 64, CV_8UC3);
    original.frame.at<cv::Vec3b>(10, 20) = {255, 128, 64};
    original.frame_id = 42;
    original.timestamp = std::chrono::steady_clock::time_point(
        std::chrono::nanoseconds(987654321));
    original.client_id = "test-client";

    auto pb = to_proto(original);
    auto restored = from_proto(pb);

    EXPECT_EQ(restored.frame_id, original.frame_id);
    EXPECT_EQ(restored.timestamp, original.timestamp);
    EXPECT_EQ(restored.client_id, original.client_id);
    ASSERT_FALSE(restored.frame.empty());
    EXPECT_EQ(restored.frame.rows, original.frame.rows);
    EXPECT_EQ(restored.frame.cols, original.frame.cols);
}

// ---- GazeEvent (feature_data omitted as FeatureData is an aggregate proto) ----

TEST(ProtoRoundTrip, gaze_event) {
    auto ts = std::chrono::steady_clock::time_point(std::chrono::nanoseconds(111222333));
    GazeResult original{
        .gaze_point = {0.3F, 0.8F},
        .confidence = 0.92F,
        .blink = BlinkType::Double,
        .timestamp = ts,
    };

    auto pb = to_proto(original, "gaze-client", 99);
    EXPECT_EQ(pb.client_id(), "gaze-client");
    EXPECT_EQ(pb.frame_id(), 99U);

    auto restored = from_proto(pb);
    EXPECT_FLOAT_EQ(restored.gaze_point.x, 0.3F);
    EXPECT_FLOAT_EQ(restored.gaze_point.y, 0.8F);
    EXPECT_EQ(restored.blink, BlinkType::Double);
}

// ---- CalibrationPoint (via CalibrationRequest) ----

TEST(ProtoRoundTrip, calibration_point) {
    CalibrationProfile cp;
    cp.user_id = "user-1";
    cp.screen_points = {{0.1F, 0.2F}};
    cp.pupil_observations = {{.center = {100.0F, 200.0F}, .radius = 12.0F, .confidence = 0.9F}};

    auto pb = to_proto(cp);
    ASSERT_EQ(pb.points_size(), 1);
    auto restored = from_proto(pb);
    ASSERT_EQ(restored.screen_points.size(), 1U);
    EXPECT_FLOAT_EQ(restored.screen_points[0].x, 0.1F);
    ASSERT_EQ(restored.pupil_observations.size(), 1U);
    EXPECT_FLOAT_EQ(restored.pupil_observations[0].radius, 12.0F);
}

// ---- CalibrationRequest ----

TEST(ProtoRoundTrip, calibration_request) {
    CalibrationProfile original;
    original.user_id = "test-user";
    original.screen_points = {{0.0F, 0.0F}, {0.5F, 0.5F}, {1.0F, 1.0F}};
    original.pupil_observations = {
        {.center = {100.0F, 100.0F}, .radius = 10.0F, .confidence = 0.9F},
        {.center = {150.0F, 150.0F}, .radius = 11.0F, .confidence = 0.85F},
        {.center = {200.0F, 200.0F}, .radius = 12.0F, .confidence = 0.92F},
    };

    auto pb = to_proto(original);
    auto restored = from_proto(pb);

    EXPECT_EQ(restored.user_id, original.user_id);
    ASSERT_EQ(restored.screen_points.size(), 3U);
    ASSERT_EQ(restored.pupil_observations.size(), 3U);
    EXPECT_FLOAT_EQ(restored.screen_points[1].x, 0.5F);
    EXPECT_FLOAT_EQ(restored.pupil_observations[2].radius, 12.0F);
}

// ---- CalibrationResponse ----

TEST(ProtoRoundTrip, calibration_response) {
    proto::CalibrationResponse pb;
    pb.set_success(true);
    pb.set_error_message("");
    pb.set_profile_id("profile-123");

    // Round-trip via serialize/deserialize
    std::string bytes;
    ASSERT_TRUE(pb.SerializeToString(&bytes));

    proto::CalibrationResponse restored;
    ASSERT_TRUE(restored.ParseFromString(bytes));
    EXPECT_TRUE(restored.success());
    EXPECT_TRUE(restored.error_message().empty());
    EXPECT_EQ(restored.profile_id(), "profile-123");
}

// ---- Binary serialization ----

TEST(ProtoSerialization, binary_serialize_deserialize) {
    Point2D original{42.5F, 99.1F};
    auto pb = to_proto(original);

    std::string bytes;
    ASSERT_TRUE(pb.SerializeToString(&bytes));
    ASSERT_FALSE(bytes.empty());

    proto::Point2D restored;
    ASSERT_TRUE(restored.ParseFromString(bytes));
    EXPECT_FLOAT_EQ(restored.x(), 42.5F);
    EXPECT_FLOAT_EQ(restored.y(), 99.1F);
}

// ---- Empty message ----

TEST(ProtoSerialization, empty_message_roundtrip) {
    proto::GazeEvent empty;
    std::string bytes;
    ASSERT_TRUE(empty.SerializeToString(&bytes));

    proto::GazeEvent restored;
    ASSERT_TRUE(restored.ParseFromString(bytes));
    EXPECT_FLOAT_EQ(restored.confidence(), 0.0F);
    EXPECT_EQ(restored.blink(), proto::BLINK_NONE);
}

}  // namespace

#else
// No-op when gRPC is not enabled
#include <gtest/gtest.h>
TEST(ProtoSerialization, skipped_no_grpc) {
    GTEST_SKIP() << "Protobuf tests require EYETRACK_ENABLE_GRPC=ON";
}
#endif  // EYETRACK_HAS_GRPC
