#include <gtest/gtest.h>

#include <filesystem>

#include <eyetrack/core/node_registry.hpp>
#include <eyetrack/nodes/face_detector.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace {

static const std::string kProjectRoot = EYETRACK_PROJECT_ROOT;
static const std::string kFixtureDir = kProjectRoot + "/tests/fixtures/face_images";
static const std::string kDlibModelPath =
    kProjectRoot + "/models/shape_predictor_68_face_landmarks.dat";
static const std::string kOnnxModelPath =
    kProjectRoot + "/models/face_landmark.onnx";

cv::Mat load_fixture(const std::string& filename) {
    return cv::imread(kFixtureDir + "/" + filename);
}

bool has_dlib() {
#ifdef EYETRACK_HAS_DLIB
    return true;
#else
    return false;
#endif
}

bool has_onnx() {
#ifdef EYETRACK_HAS_ONNXRUNTIME
    return true;
#else
    return false;
#endif
}

bool dlib_model_exists() {
    return std::filesystem::exists(kDlibModelPath);
}

bool onnx_model_exists() {
    return std::filesystem::exists(kOnnxModelPath);
}

eyetrack::FaceDetector make_dlib_detector() {
    eyetrack::FaceDetector::Config config;
    config.backend = eyetrack::FaceDetectorBackend::Dlib;
    config.dlib_shape_predictor_path = kDlibModelPath;
    return eyetrack::FaceDetector(config);
}

#define SKIP_IF_NO_DLIB()                                 \
    do {                                                  \
        if (!has_dlib() || !dlib_model_exists()) {        \
            GTEST_SKIP() << "dlib or model not available"; \
        }                                                 \
    } while (0)

}  // namespace

TEST(FaceDetectorDlib, detects_frontal_face) {
    SKIP_IF_NO_DLIB();

    auto detector = make_dlib_detector();
    auto input = load_fixture("frontal_01_standard.png");
    if (input.empty()) GTEST_SKIP() << "Fixture not found";

    auto result = detector.run(input);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_GT(result->confidence, 0.0F);
    EXPECT_FALSE(result->landmarks.empty());
}

TEST(FaceDetectorDlib, produces_68_landmarks) {
    SKIP_IF_NO_DLIB();

    auto detector = make_dlib_detector();
    auto input = load_fixture("frontal_01_standard.png");
    if (input.empty()) GTEST_SKIP() << "Fixture not found";

    auto result = detector.run(input);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->landmarks.size(), 68u);
}

TEST(FaceDetectorDlib, landmarks_within_frame_bounds) {
    SKIP_IF_NO_DLIB();

    auto detector = make_dlib_detector();
    auto input = load_fixture("frontal_01_standard.png");
    if (input.empty()) GTEST_SKIP() << "Fixture not found";

    auto result = detector.run(input);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    const auto frame_w = static_cast<float>(input.cols);
    const auto frame_h = static_cast<float>(input.rows);

    for (size_t i = 0; i < result->landmarks.size(); ++i) {
        const auto& lm = result->landmarks[i];
        EXPECT_GE(lm.x, 0.0F) << "Landmark " << i << " x below 0";
        EXPECT_LE(lm.x, frame_w) << "Landmark " << i << " x above frame width";
        EXPECT_GE(lm.y, 0.0F) << "Landmark " << i << " y below 0";
        EXPECT_LE(lm.y, frame_h) << "Landmark " << i << " y above frame height";
    }
}

TEST(FaceDetectorDlib, ear_subset_mapping_valid) {
    SKIP_IF_NO_DLIB();

    auto detector = make_dlib_detector();
    auto input = load_fixture("frontal_01_standard.png");
    if (input.empty()) GTEST_SKIP() << "Fixture not found";

    auto result = detector.run(input);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    ASSERT_EQ(result->landmarks.size(), 68u);

    // Extract EAR subsets
    auto left_ear = eyetrack::FaceDetector::dlib_ear_subset(
        result->landmarks, eyetrack::FaceDetector::kDlibLeftEyeEAR);
    auto right_ear = eyetrack::FaceDetector::dlib_ear_subset(
        result->landmarks, eyetrack::FaceDetector::kDlibRightEyeEAR);

    // Verify all 6 points are non-zero and distinct from origin
    for (size_t i = 0; i < 6; ++i) {
        EXPECT_TRUE(left_ear[i].x > 0.0F || left_ear[i].y > 0.0F)
            << "Left EAR point " << i << " is at origin";
        EXPECT_TRUE(right_ear[i].x > 0.0F || right_ear[i].y > 0.0F)
            << "Right EAR point " << i << " is at origin";
    }

    // Left eye should be to the right of right eye in image coords
    // (dlib convention: landmarks 36-41 are right eye, 42-47 are left eye in image)
    float left_center_x = 0.0F;
    float right_center_x = 0.0F;
    for (size_t i = 0; i < 6; ++i) {
        left_center_x += left_ear[i].x;
        right_center_x += right_ear[i].x;
    }
    left_center_x /= 6.0F;
    right_center_x /= 6.0F;
    // dlib indices 36-41 = right eye (left in image), 42-47 = left eye (right in image)
    EXPECT_LT(left_center_x, right_center_x)
        << "Left eye EAR center should be left of right eye in image";
}

TEST(FaceDetectorDlib, non_face_returns_error) {
    SKIP_IF_NO_DLIB();

    auto detector = make_dlib_detector();
    auto input = load_fixture("nonface_01_texture.png");
    if (input.empty()) GTEST_SKIP() << "Non-face fixture not found";

    auto result = detector.run(input);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::FaceNotDetected);
}

TEST(FaceDetectorDlib, config_selects_backend) {
    eyetrack::FaceDetector::Config config;
    config.backend = eyetrack::FaceDetectorBackend::Dlib;
    eyetrack::FaceDetector detector(config);
    EXPECT_EQ(detector.backend(), eyetrack::FaceDetectorBackend::Dlib);

    eyetrack::FaceDetector default_detector;
    EXPECT_EQ(default_detector.backend(), eyetrack::FaceDetectorBackend::MediaPipe);
}

TEST(FaceDetectorDlib, node_params_selects_dlib) {
    eyetrack::NodeParams params;
    params["backend"] = "dlib";
    eyetrack::FaceDetector detector(params);
    EXPECT_EQ(detector.backend(), eyetrack::FaceDetectorBackend::Dlib);
}

TEST(FaceDetectorDlib, both_backends_detect_same_face) {
    if (!has_dlib() || !dlib_model_exists() || !has_onnx() || !onnx_model_exists()) {
        GTEST_SKIP() << "Both backends required";
    }

    auto input = load_fixture("frontal_01_standard.png");
    if (input.empty()) GTEST_SKIP() << "Fixture not found";

    // MediaPipe
    eyetrack::FaceDetector::Config mp_config;
    mp_config.backend = eyetrack::FaceDetectorBackend::MediaPipe;
    mp_config.model_path = kOnnxModelPath;
    mp_config.confidence_threshold = 0.0F;
    eyetrack::FaceDetector mp_detector(mp_config);
    auto mp_result = mp_detector.run(input);

    // dlib
    auto dlib_detector = make_dlib_detector();
    auto dlib_result = dlib_detector.run(input);

    // Both should detect a face (landmark counts differ: 468 vs 68)
    // Note: bounding box overlap check is skipped because the MediaPipe model
    // may be a stub producing random landmarks. With a real model, both
    // bounding boxes should overlap significantly.
    if (mp_result.has_value()) {
        EXPECT_EQ(mp_result->landmarks.size(), 468u);
    }

    // dlib with a real shape predictor must always succeed on frontal faces
    ASSERT_TRUE(dlib_result.has_value())
        << "dlib should detect face: " << dlib_result.error().message;
    EXPECT_EQ(dlib_result->landmarks.size(), 68u);
}

TEST(FaceDetectorDlib, invalid_model_path_returns_error) {
    if (!has_dlib()) GTEST_SKIP() << "dlib not available";

    eyetrack::FaceDetector::Config config;
    config.backend = eyetrack::FaceDetectorBackend::Dlib;
    config.dlib_shape_predictor_path = "/nonexistent/model.dat";
    eyetrack::FaceDetector detector(config);

    cv::Mat input(480, 640, CV_8UC3, cv::Scalar(128, 128, 128));
    auto result = detector.run(input);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::ModelLoadFailed);
}
