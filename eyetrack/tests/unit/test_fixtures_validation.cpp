#include <gtest/gtest.h>

#include <filesystem>
#include <opencv2/imgcodecs.hpp>
#include <string>
#include <vector>

#ifdef EYETRACK_HAS_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif

namespace fs = std::filesystem;

static const std::string kProjectRoot = EYETRACK_PROJECT_ROOT;
static const std::string kFaceImagesDir = kProjectRoot + "/tests/fixtures/face_images";
static const std::string kModelsDir = kProjectRoot + "/models";

// Collect frontal face images (files matching "frontal_*")
static std::vector<std::string> get_frontal_images() {
    std::vector<std::string> images;
    if (!fs::exists(kFaceImagesDir)) return images;
    for (const auto& entry : fs::directory_iterator(kFaceImagesDir)) {
        if (entry.path().filename().string().starts_with("frontal_")) {
            images.push_back(entry.path().string());
        }
    }
    std::sort(images.begin(), images.end());
    return images;
}

// Collect non-face images (files matching "nonface_*")
static std::vector<std::string> get_nonface_images() {
    std::vector<std::string> images;
    if (!fs::exists(kFaceImagesDir)) return images;
    for (const auto& entry : fs::directory_iterator(kFaceImagesDir)) {
        if (entry.path().filename().string().starts_with("nonface_")) {
            images.push_back(entry.path().string());
        }
    }
    std::sort(images.begin(), images.end());
    return images;
}

TEST(Fixtures, face_images_exist) {
    auto images = get_frontal_images();
    ASSERT_GE(images.size(), 5u) << "Need at least 5 frontal face images in " << kFaceImagesDir;
    for (const auto& path : images) {
        EXPECT_TRUE(fs::exists(path)) << "Missing: " << path;
    }
}

TEST(Fixtures, non_face_images_exist) {
    auto images = get_nonface_images();
    ASSERT_GE(images.size(), 2u) << "Need at least 2 non-face images in " << kFaceImagesDir;
    for (const auto& path : images) {
        EXPECT_TRUE(fs::exists(path)) << "Missing: " << path;
    }
}

TEST(Fixtures, face_images_loadable_by_opencv) {
    auto images = get_frontal_images();
    ASSERT_FALSE(images.empty());
    for (const auto& path : images) {
        cv::Mat img = cv::imread(path);
        EXPECT_FALSE(img.empty()) << "Failed to load: " << path;
        EXPECT_EQ(img.channels(), 3) << "Expected BGR (3 channels): " << path;
    }

    auto nonface = get_nonface_images();
    for (const auto& path : nonface) {
        cv::Mat img = cv::imread(path);
        EXPECT_FALSE(img.empty()) << "Failed to load: " << path;
    }
}

TEST(Fixtures, face_images_minimum_resolution) {
    auto images = get_frontal_images();
    ASSERT_FALSE(images.empty());
    for (const auto& path : images) {
        cv::Mat img = cv::imread(path);
        ASSERT_FALSE(img.empty()) << "Failed to load: " << path;
        EXPECT_GE(img.cols, 64) << "Width < 64: " << path;
        EXPECT_GE(img.rows, 64) << "Height < 64: " << path;
    }
}

TEST(Fixtures, onnx_model_file_exists) {
    const std::string model_path = kModelsDir + "/face_landmark.onnx";
    ASSERT_TRUE(fs::exists(model_path)) << "Missing: " << model_path;
    EXPECT_GT(fs::file_size(model_path), 0u) << "Model file is empty";
}

TEST(Fixtures, onnx_model_loads) {
#ifndef EYETRACK_HAS_ONNXRUNTIME
    GTEST_SKIP() << "ONNX Runtime not available";
#else
    const std::string model_path = kModelsDir + "/face_landmark.onnx";
    ASSERT_TRUE(fs::exists(model_path));

    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "test");
    Ort::SessionOptions session_options;
    ASSERT_NO_THROW({
        Ort::Session session(env, model_path.c_str(), session_options);
    }) << "Failed to load ONNX model: " << model_path;
#endif
}

TEST(Fixtures, onnx_model_input_shape) {
#ifndef EYETRACK_HAS_ONNXRUNTIME
    GTEST_SKIP() << "ONNX Runtime not available";
#else
    const std::string model_path = kModelsDir + "/face_landmark.onnx";
    ASSERT_TRUE(fs::exists(model_path));

    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "test");
    Ort::SessionOptions session_options;
    Ort::Session session(env, model_path.c_str(), session_options);

    // Verify input shape
    auto input_info = session.GetInputTypeInfo(0);
    auto tensor_info = input_info.GetTensorTypeAndShapeInfo();
    auto shape = tensor_info.GetShape();

    // Expected: [1, 192, 192, 3] (NHWC) or [1, 3, 192, 192] (NCHW)
    ASSERT_EQ(shape.size(), 4u) << "Expected 4D input tensor";
    EXPECT_EQ(shape[0], 1) << "Batch size should be 1";

    // Check for either NHWC or NCHW layout
    bool is_nhwc = (shape[1] == 192 && shape[2] == 192 && shape[3] == 3);
    bool is_nchw = (shape[1] == 3 && shape[2] == 192 && shape[3] == 192);
    EXPECT_TRUE(is_nhwc || is_nchw)
        << "Expected [1,192,192,3] or [1,3,192,192], got ["
        << shape[0] << "," << shape[1] << "," << shape[2] << "," << shape[3] << "]";
#endif
}

TEST(Fixtures, models_readme_exists) {
    const std::string readme_path = kModelsDir + "/README.md";
    ASSERT_TRUE(fs::exists(readme_path)) << "Missing: " << readme_path;
    EXPECT_GT(fs::file_size(readme_path), 0u) << "README.md is empty";
}
