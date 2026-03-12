#include <gtest/gtest.h>

#include <opencv2/core.hpp>

#include <eyetrack/pipeline/pipeline_context.hpp>

TEST(PipelineContext, stores_frame_data) {
    eyetrack::PipelineContext ctx;

    eyetrack::FrameData fd;
    fd.frame_id = 42;
    fd.frame = cv::Mat::ones(480, 640, CV_8UC3);
    ctx.set_frame_data(fd);

    auto retrieved = ctx.frame_data();
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->frame_id, 42U);
    EXPECT_EQ(retrieved->frame.rows, 480);

    // Also verify the raw "frame" key is set (for PipelineNodeBase compat)
    auto& raw = ctx.raw();
    EXPECT_TRUE(raw.contains("frame"));
}

TEST(PipelineContext, stores_face_roi) {
    eyetrack::PipelineContext ctx;

    eyetrack::FaceROI roi;
    roi.bounding_box = cv::Rect2f(10.0F, 20.0F, 100.0F, 120.0F);
    roi.confidence = 0.95F;
    ctx.set_face_roi(roi);

    auto retrieved = ctx.face_roi();
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_FLOAT_EQ(retrieved->confidence, 0.95F);
    EXPECT_FLOAT_EQ(retrieved->bounding_box.x, 10.0F);
}

TEST(PipelineContext, stores_eye_landmarks) {
    eyetrack::PipelineContext ctx;

    eyetrack::EyeLandmarks lm;
    lm.left[0] = {1.0F, 2.0F};
    lm.right[0] = {3.0F, 4.0F};
    ctx.set_eye_landmarks(lm);

    auto retrieved = ctx.eye_landmarks();
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_FLOAT_EQ(retrieved->left[0].x, 1.0F);
    EXPECT_FLOAT_EQ(retrieved->right[0].x, 3.0F);
}

TEST(PipelineContext, stores_pupil_info) {
    eyetrack::PipelineContext ctx;

    eyetrack::PupilInfo left;
    left.center = {50.0F, 60.0F};
    left.radius = 5.0F;
    left.confidence = 0.8F;
    ctx.set_left_pupil(left);

    eyetrack::PupilInfo right;
    right.center = {150.0F, 60.0F};
    right.radius = 4.5F;
    right.confidence = 0.85F;
    ctx.set_right_pupil(right);

    auto lp = ctx.left_pupil();
    ASSERT_TRUE(lp.has_value());
    EXPECT_FLOAT_EQ(lp->center.x, 50.0F);

    auto rp = ctx.right_pupil();
    ASSERT_TRUE(rp.has_value());
    EXPECT_FLOAT_EQ(rp->center.x, 150.0F);
}

TEST(PipelineContext, stores_gaze_result) {
    eyetrack::PipelineContext ctx;

    eyetrack::GazeResult result;
    result.gaze_point = {0.5F, 0.7F};
    result.confidence = 0.9F;
    result.blink = eyetrack::BlinkType::None;
    ctx.set_gaze_result(result);

    auto retrieved = ctx.gaze_result();
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_FLOAT_EQ(retrieved->gaze_point.x, 0.5F);
    EXPECT_FLOAT_EQ(retrieved->gaze_point.y, 0.7F);
    EXPECT_FLOAT_EQ(retrieved->confidence, 0.9F);
}

TEST(PipelineContext, clear_resets_all_fields) {
    eyetrack::PipelineContext ctx;

    // Set all fields
    eyetrack::FrameData fd;
    fd.frame = cv::Mat::ones(100, 100, CV_8UC3);
    ctx.set_frame_data(fd);
    ctx.set_face_roi(eyetrack::FaceROI{});
    ctx.set_eye_landmarks(eyetrack::EyeLandmarks{});
    ctx.set_left_pupil(eyetrack::PupilInfo{});
    ctx.set_right_pupil(eyetrack::PupilInfo{});
    ctx.set_gaze_result(eyetrack::GazeResult{});
    ctx.set_ear(0.3F);
    ctx.set_blink_type(eyetrack::BlinkType::Single);

    // Clear
    ctx.clear();

    // All should be empty
    EXPECT_FALSE(ctx.frame_data().has_value());
    EXPECT_FALSE(ctx.face_roi().has_value());
    EXPECT_FALSE(ctx.eye_landmarks().has_value());
    EXPECT_FALSE(ctx.left_pupil().has_value());
    EXPECT_FALSE(ctx.right_pupil().has_value());
    EXPECT_FALSE(ctx.gaze_result().has_value());
    EXPECT_FALSE(ctx.ear().has_value());
    EXPECT_FALSE(ctx.blink_type().has_value());
    EXPECT_TRUE(ctx.raw().empty());
}
