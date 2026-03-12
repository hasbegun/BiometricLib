#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <utility>

#include <eyetrack/core/types.hpp>

namespace {

using namespace eyetrack;

// ---- Point2D ----

TEST(Point2D, default_construction_zero) {
    Point2D p;
    EXPECT_FLOAT_EQ(p.x, 0.0F);
    EXPECT_FLOAT_EQ(p.y, 0.0F);
}

TEST(Point2D, parameterized_construction) {
    Point2D p{1.5F, 2.5F};
    EXPECT_FLOAT_EQ(p.x, 1.5F);
    EXPECT_FLOAT_EQ(p.y, 2.5F);
}

TEST(Point2D, equality_operator) {
    Point2D a{1.0F, 2.0F};
    Point2D b{1.0F, 2.0F};
    Point2D c{3.0F, 4.0F};
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(Point2D, move_semantics) {
    Point2D original{5.0F, 10.0F};
    Point2D moved(std::move(original));
    EXPECT_FLOAT_EQ(moved.x, 5.0F);
    EXPECT_FLOAT_EQ(moved.y, 10.0F);
}

// ---- EyeLandmarks ----

TEST(EyeLandmarks, default_construction) {
    EyeLandmarks lm;
    for (size_t i = 0; i < 6; ++i) {
        EXPECT_FLOAT_EQ(lm.left[i].x, 0.0F);
        EXPECT_FLOAT_EQ(lm.left[i].y, 0.0F);
        EXPECT_FLOAT_EQ(lm.right[i].x, 0.0F);
        EXPECT_FLOAT_EQ(lm.right[i].y, 0.0F);
    }
}

TEST(EyeLandmarks, array_indexing) {
    EyeLandmarks lm;
    lm.left[0] = {1.0F, 2.0F};
    lm.left[5] = {11.0F, 12.0F};
    lm.right[3] = {7.0F, 8.0F};

    EXPECT_FLOAT_EQ(lm.left[0].x, 1.0F);
    EXPECT_FLOAT_EQ(lm.left[5].y, 12.0F);
    EXPECT_FLOAT_EQ(lm.right[3].x, 7.0F);
}

// ---- PupilInfo ----

TEST(PupilInfo, construction_and_fields) {
    PupilInfo pi{.center = {100.0F, 200.0F}, .radius = 15.0F, .confidence = 0.95F};
    EXPECT_FLOAT_EQ(pi.center.x, 100.0F);
    EXPECT_FLOAT_EQ(pi.center.y, 200.0F);
    EXPECT_FLOAT_EQ(pi.radius, 15.0F);
    EXPECT_FLOAT_EQ(pi.confidence, 0.95F);
}

// ---- BlinkType ----

TEST(BlinkType, enum_values) {
    EXPECT_NE(static_cast<uint8_t>(BlinkType::None), static_cast<uint8_t>(BlinkType::Single));
    EXPECT_NE(static_cast<uint8_t>(BlinkType::Single), static_cast<uint8_t>(BlinkType::Double));
    EXPECT_NE(static_cast<uint8_t>(BlinkType::Double), static_cast<uint8_t>(BlinkType::Long));
    EXPECT_EQ(static_cast<uint8_t>(BlinkType::None), 0);
    EXPECT_EQ(static_cast<uint8_t>(BlinkType::Single), 1);
    EXPECT_EQ(static_cast<uint8_t>(BlinkType::Double), 2);
    EXPECT_EQ(static_cast<uint8_t>(BlinkType::Long), 3);
}

// ---- GazeResult ----

TEST(GazeResult, construction_and_fields) {
    auto now = std::chrono::steady_clock::now();
    GazeResult gr{
        .gaze_point = {0.5F, 0.7F},
        .confidence = 0.85F,
        .blink = BlinkType::Single,
        .timestamp = now,
    };
    EXPECT_FLOAT_EQ(gr.gaze_point.x, 0.5F);
    EXPECT_FLOAT_EQ(gr.gaze_point.y, 0.7F);
    EXPECT_FLOAT_EQ(gr.confidence, 0.85F);
    EXPECT_EQ(gr.blink, BlinkType::Single);
    EXPECT_EQ(gr.timestamp, now);
}

// ---- FaceROI ----

TEST(FaceROI, bounding_box_and_landmarks) {
    FaceROI roi;
    roi.bounding_box = cv::Rect2f(10.0F, 20.0F, 100.0F, 120.0F);
    roi.confidence = 0.99F;

    // Simulate 468 MediaPipe landmarks
    roi.landmarks.resize(468);
    roi.landmarks[0] = {0.1F, 0.2F};
    roi.landmarks[467] = {0.9F, 0.8F};

    EXPECT_FLOAT_EQ(roi.bounding_box.x, 10.0F);
    EXPECT_FLOAT_EQ(roi.bounding_box.width, 100.0F);
    EXPECT_EQ(roi.landmarks.size(), 468U);
    EXPECT_FLOAT_EQ(roi.landmarks[0].x, 0.1F);
    EXPECT_FLOAT_EQ(roi.landmarks[467].y, 0.8F);
    EXPECT_FLOAT_EQ(roi.confidence, 0.99F);
}

// ---- FrameData ----

TEST(FrameData, frame_id_and_timestamp) {
    auto now = std::chrono::steady_clock::now();
    FrameData fd;
    fd.frame = cv::Mat::zeros(480, 640, CV_8UC3);
    fd.frame_id = 42;
    fd.timestamp = now;
    fd.client_id = "test-client";

    EXPECT_EQ(fd.frame_id, 42U);
    EXPECT_EQ(fd.timestamp, now);
    EXPECT_EQ(fd.client_id, "test-client");
    EXPECT_EQ(fd.frame.rows, 480);
    EXPECT_EQ(fd.frame.cols, 640);
}

// ---- CalibrationProfile ----

TEST(CalibrationProfile, no_eigen_dependency) {
    // Compile-time verification: types.hpp should not include Eigen.
    // If it did, we'd see compilation errors due to missing Eigen headers
    // in a build without Eigen include paths for this specific test.
    // For runtime check: verify the struct has no Eigen-typed fields.
    CalibrationProfile cp;
    cp.user_id = "user-1";
    cp.screen_points = {{0.0F, 0.0F}, {1.0F, 1.0F}};
    cp.pupil_observations = {
        {.center = {100.0F, 100.0F}, .radius = 10.0F, .confidence = 0.9F}};

    // If this compiles and runs, types.hpp does not require Eigen.
    SUCCEED();
}

TEST(CalibrationProfile, user_id_and_points) {
    CalibrationProfile cp;
    cp.user_id = "test-user";
    cp.screen_points = {{0.1F, 0.1F}, {0.5F, 0.5F}, {0.9F, 0.9F}};
    cp.pupil_observations = {
        {.center = {100.0F, 200.0F}, .radius = 12.0F, .confidence = 0.9F},
        {.center = {150.0F, 250.0F}, .radius = 13.0F, .confidence = 0.85F},
        {.center = {200.0F, 300.0F}, .radius = 11.0F, .confidence = 0.92F},
    };

    EXPECT_EQ(cp.user_id, "test-user");
    EXPECT_EQ(cp.screen_points.size(), 3U);
    EXPECT_EQ(cp.pupil_observations.size(), 3U);
    EXPECT_FLOAT_EQ(cp.screen_points[1].x, 0.5F);
    EXPECT_FLOAT_EQ(cp.pupil_observations[2].radius, 11.0F);
}

// ---- Stream operators ----

TEST(Types, stream_operators) {
    std::ostringstream oss;

    oss << Point2D{1.0F, 2.0F};
    EXPECT_FALSE(oss.str().empty());
    oss.str("");

    oss << EyeLandmarks{};
    EXPECT_FALSE(oss.str().empty());
    oss.str("");

    oss << PupilInfo{};
    EXPECT_FALSE(oss.str().empty());
    oss.str("");

    oss << BlinkType::Single;
    EXPECT_EQ(oss.str(), "Single");
    oss.str("");

    oss << BlinkType::Double;
    EXPECT_EQ(oss.str(), "Double");
    oss.str("");

    oss << BlinkType::Long;
    EXPECT_EQ(oss.str(), "Long");
    oss.str("");

    oss << BlinkType::None;
    EXPECT_EQ(oss.str(), "None");
    oss.str("");

    oss << GazeResult{};
    EXPECT_FALSE(oss.str().empty());
    oss.str("");

    oss << FaceROI{};
    EXPECT_FALSE(oss.str().empty());
    oss.str("");

    oss << FrameData{};
    EXPECT_FALSE(oss.str().empty());
    oss.str("");

    oss << CalibrationProfile{};
    EXPECT_FALSE(oss.str().empty());
}

}  // namespace
