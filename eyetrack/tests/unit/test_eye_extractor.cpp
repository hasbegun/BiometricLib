#include <gtest/gtest.h>

#include <eyetrack/core/node_registry.hpp>
#include <eyetrack/nodes/eye_extractor.hpp>
#include <opencv2/imgproc.hpp>

namespace {

// Create a synthetic FaceROI with 468 landmarks spread across the frame.
// Eye landmarks are placed at plausible positions for a 640x480 frame.
eyetrack::FaceROI make_mediapipe_face_roi(int frame_w, int frame_h) {
    eyetrack::FaceROI face;
    face.confidence = 0.9F;
    face.landmarks.resize(468);

    // Fill all landmarks with center-ish positions
    for (int i = 0; i < 468; ++i) {
        face.landmarks[static_cast<size_t>(i)] = {
            static_cast<float>(frame_w) * 0.5F,
            static_cast<float>(frame_h) * 0.5F};
    }

    // Place left eye landmarks (MediaPipe indices: 362, 385, 387, 263, 373, 380)
    // in the right side of the frame (person's left eye = viewer's right)
    face.landmarks[362] = {380.0F, 200.0F};  // lateral corner
    face.landmarks[385] = {370.0F, 190.0F};  // upper lid 1
    face.landmarks[387] = {360.0F, 188.0F};  // upper lid 2
    face.landmarks[263] = {340.0F, 200.0F};  // medial corner
    face.landmarks[373] = {360.0F, 212.0F};  // lower lid 1
    face.landmarks[380] = {370.0F, 210.0F};  // lower lid 2

    // Place right eye landmarks (MediaPipe indices: 33, 160, 158, 133, 153, 144)
    // in the left side of the frame (person's right eye = viewer's left)
    face.landmarks[33]  = {260.0F, 200.0F};  // lateral corner
    face.landmarks[160] = {270.0F, 190.0F};  // upper lid 1
    face.landmarks[158] = {280.0F, 188.0F};  // upper lid 2
    face.landmarks[133] = {300.0F, 200.0F};  // medial corner
    face.landmarks[153] = {280.0F, 212.0F};  // lower lid 1
    face.landmarks[144] = {270.0F, 210.0F};  // lower lid 2

    face.bounding_box = cv::Rect2f(200.0F, 100.0F, 240.0F, 300.0F);
    return face;
}

// Create a synthetic FaceROI with 68 dlib landmarks.
eyetrack::FaceROI make_dlib_face_roi(int frame_w, int frame_h) {
    eyetrack::FaceROI face;
    face.confidence = 1.0F;
    face.landmarks.resize(68);

    for (int i = 0; i < 68; ++i) {
        face.landmarks[static_cast<size_t>(i)] = {
            static_cast<float>(frame_w) * 0.5F,
            static_cast<float>(frame_h) * 0.5F};
    }

    // dlib left eye: indices 36-41
    face.landmarks[36] = {250.0F, 200.0F};
    face.landmarks[37] = {260.0F, 192.0F};
    face.landmarks[38] = {270.0F, 190.0F};
    face.landmarks[39] = {280.0F, 200.0F};
    face.landmarks[40] = {270.0F, 208.0F};
    face.landmarks[41] = {260.0F, 208.0F};

    // dlib right eye: indices 42-47
    face.landmarks[42] = {340.0F, 200.0F};
    face.landmarks[43] = {350.0F, 192.0F};
    face.landmarks[44] = {360.0F, 190.0F};
    face.landmarks[45] = {370.0F, 200.0F};
    face.landmarks[46] = {360.0F, 208.0F};
    face.landmarks[47] = {350.0F, 208.0F};

    face.bounding_box = cv::Rect2f(200.0F, 100.0F, 240.0F, 300.0F);
    return face;
}

cv::Mat make_test_frame(int width = 640, int height = 480) {
    cv::Mat frame(height, width, CV_8UC3, cv::Scalar(128, 128, 128));
    return frame;
}

}  // namespace

// --- MediaPipe landmark tests ---

TEST(EyeExtractor, left_eye_6_points) {
    auto face = make_mediapipe_face_roi(640, 480);
    auto frame = make_test_frame();
    eyetrack::EyeExtractor extractor;

    auto result = extractor.run(face, frame);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // left eye should have exactly 6 points
    const auto& left = result->landmarks.left;
    // All 6 points should be non-default
    int non_zero = 0;
    for (const auto& pt : left) {
        if (pt.x != 0.0F || pt.y != 0.0F) ++non_zero;
    }
    EXPECT_EQ(non_zero, 6);
}

TEST(EyeExtractor, right_eye_6_points) {
    auto face = make_mediapipe_face_roi(640, 480);
    auto frame = make_test_frame();
    eyetrack::EyeExtractor extractor;

    auto result = extractor.run(face, frame);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    int non_zero = 0;
    for (const auto& pt : result->landmarks.right) {
        if (pt.x != 0.0F || pt.y != 0.0F) ++non_zero;
    }
    EXPECT_EQ(non_zero, 6);
}

TEST(EyeExtractor, left_eye_indices_correct) {
    auto face = make_mediapipe_face_roi(640, 480);
    auto frame = make_test_frame();
    eyetrack::EyeExtractor extractor;

    auto result = extractor.run(face, frame);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Verify left eye EAR matches the MediaPipe indices
    constexpr std::array<int, 6> expected_indices = {362, 385, 387, 263, 373, 380};
    for (size_t i = 0; i < 6; ++i) {
        auto idx = static_cast<size_t>(expected_indices[i]);
        EXPECT_FLOAT_EQ(result->landmarks.left[i].x, face.landmarks[idx].x)
            << "Left eye point " << i << " x mismatch";
        EXPECT_FLOAT_EQ(result->landmarks.left[i].y, face.landmarks[idx].y)
            << "Left eye point " << i << " y mismatch";
    }
}

TEST(EyeExtractor, right_eye_indices_correct) {
    auto face = make_mediapipe_face_roi(640, 480);
    auto frame = make_test_frame();
    eyetrack::EyeExtractor extractor;

    auto result = extractor.run(face, frame);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    constexpr std::array<int, 6> expected_indices = {33, 160, 158, 133, 153, 144};
    for (size_t i = 0; i < 6; ++i) {
        auto idx = static_cast<size_t>(expected_indices[i]);
        EXPECT_FLOAT_EQ(result->landmarks.right[i].x, face.landmarks[idx].x)
            << "Right eye point " << i << " x mismatch";
        EXPECT_FLOAT_EQ(result->landmarks.right[i].y, face.landmarks[idx].y)
            << "Right eye point " << i << " y mismatch";
    }
}

// --- Crop tests ---

TEST(EyeExtractor, crop_dimensions_with_default_padding) {
    auto face = make_mediapipe_face_roi(640, 480);
    auto frame = make_test_frame();
    eyetrack::EyeExtractor extractor;  // default 20% padding

    auto result = extractor.run(face, frame);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Left eye spans x:[340,380], y:[188,212] -> w=40, h=24
    // With 20% padding: pad_x=8, pad_y=4.8 -> x:[332,388], y:[183.2,216.8]
    // After floor/ceil: [332,184] to [388,217] -> 56 x 33
    EXPECT_GT(result->left_eye_crop.cols, 40);   // wider than raw bbox
    EXPECT_GT(result->left_eye_crop.rows, 24);   // taller than raw bbox
    EXPECT_GT(result->right_eye_crop.cols, 0);
    EXPECT_GT(result->right_eye_crop.rows, 0);
}

TEST(EyeExtractor, crop_dimensions_with_custom_padding) {
    auto face = make_mediapipe_face_roi(640, 480);
    auto frame = make_test_frame();

    // 10% padding
    eyetrack::EyeExtractor::Config config_10;
    config_10.padding = 0.1F;
    eyetrack::EyeExtractor extractor_10(config_10);

    // 30% padding
    eyetrack::EyeExtractor::Config config_30;
    config_30.padding = 0.3F;
    eyetrack::EyeExtractor extractor_30(config_30);

    auto result_10 = extractor_10.run(face, frame);
    auto result_30 = extractor_30.run(face, frame);
    ASSERT_TRUE(result_10.has_value()) << result_10.error().message;
    ASSERT_TRUE(result_30.has_value()) << result_30.error().message;

    // 30% padding crop should be larger than 10% padding crop
    EXPECT_GT(result_30->left_eye_crop.cols, result_10->left_eye_crop.cols);
    EXPECT_GT(result_30->left_eye_crop.rows, result_10->left_eye_crop.rows);
}

TEST(EyeExtractor, crop_bounds_clipped_to_frame) {
    auto frame = make_test_frame(640, 480);

    // Place eye landmarks near the top-left edge
    eyetrack::FaceROI face;
    face.confidence = 0.9F;
    face.landmarks.resize(468);
    for (auto& lm : face.landmarks) lm = {320.0F, 240.0F};

    // Left eye near top-left corner
    face.landmarks[362] = {10.0F, 5.0F};
    face.landmarks[385] = {8.0F, 2.0F};
    face.landmarks[387] = {5.0F, 1.0F};
    face.landmarks[263] = {2.0F, 5.0F};
    face.landmarks[373] = {5.0F, 10.0F};
    face.landmarks[380] = {8.0F, 9.0F};

    // Right eye at normal position
    face.landmarks[33]  = {260.0F, 200.0F};
    face.landmarks[160] = {270.0F, 190.0F};
    face.landmarks[158] = {280.0F, 188.0F};
    face.landmarks[133] = {300.0F, 200.0F};
    face.landmarks[153] = {280.0F, 212.0F};
    face.landmarks[144] = {270.0F, 210.0F};

    eyetrack::EyeExtractor extractor;
    auto result = extractor.run(face, frame);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Left eye crop should start at (0,0) due to clipping
    EXPECT_GT(result->left_eye_crop.cols, 0);
    EXPECT_GT(result->left_eye_crop.rows, 0);
    // Should not exceed frame dimensions
    EXPECT_LE(result->left_eye_crop.cols, frame.cols);
    EXPECT_LE(result->left_eye_crop.rows, frame.rows);
}

TEST(EyeExtractor, crop_is_non_empty) {
    auto face = make_mediapipe_face_roi(640, 480);
    auto frame = make_test_frame();
    eyetrack::EyeExtractor extractor;

    auto result = extractor.run(face, frame);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_FALSE(result->left_eye_crop.empty());
    EXPECT_FALSE(result->right_eye_crop.empty());
}

// --- Registry and error tests ---

TEST(EyeExtractor, node_registry_lookup) {
    auto& registry = eyetrack::NodeRegistry::instance();
    EXPECT_TRUE(registry.has("eyetrack.EyeExtractor"));

    auto factory = registry.lookup("eyetrack.EyeExtractor");
    ASSERT_TRUE(factory.has_value());

    eyetrack::NodeParams params;
    auto node = factory.value()(params);
    EXPECT_TRUE(node.has_value());
}

TEST(EyeExtractor, missing_landmarks_returns_error) {
    auto frame = make_test_frame();
    eyetrack::EyeExtractor extractor;

    // 100 landmarks -- neither 468 nor 68
    eyetrack::FaceROI bad_face;
    bad_face.confidence = 0.9F;
    bad_face.landmarks.resize(100);

    auto result = extractor.run(bad_face, frame);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::InvalidInput);
}

TEST(EyeExtractor, empty_frame_returns_error) {
    auto face = make_mediapipe_face_roi(640, 480);
    eyetrack::EyeExtractor extractor;

    cv::Mat empty;
    auto result = extractor.run(face, empty);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::InvalidInput);
}

// --- dlib landmark support ---

TEST(EyeExtractor, dlib_68_landmarks_work) {
    auto face = make_dlib_face_roi(640, 480);
    auto frame = make_test_frame();
    eyetrack::EyeExtractor extractor;

    auto result = extractor.run(face, frame);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Verify dlib indices were used
    EXPECT_FLOAT_EQ(result->landmarks.left[0].x, face.landmarks[36].x);
    EXPECT_FLOAT_EQ(result->landmarks.right[0].x, face.landmarks[42].x);

    EXPECT_FALSE(result->left_eye_crop.empty());
    EXPECT_FALSE(result->right_eye_crop.empty());
}

TEST(EyeExtractor, dependencies_include_face_detector) {
    eyetrack::EyeExtractor extractor;
    const auto& deps = extractor.dependencies();
    ASSERT_FALSE(deps.empty());
    EXPECT_EQ(deps[0], "eyetrack.FaceDetector");
}
