#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

#include <opencv2/imgcodecs.hpp>

namespace fs = std::filesystem;

static const std::string kProjectRoot = EYETRACK_PROJECT_ROOT;
static const std::string kEyeImagesDir = kProjectRoot + "/tests/fixtures/eye_images";

static std::vector<std::string> get_synthetic_images() {
    std::vector<std::string> images;
    if (!fs::exists(kEyeImagesDir)) return images;
    for (const auto& entry : fs::directory_iterator(kEyeImagesDir)) {
        if (entry.path().filename().string().starts_with("synthetic_")) {
            images.push_back(entry.path().string());
        }
    }
    std::sort(images.begin(), images.end());
    return images;
}

static std::vector<std::string> get_real_images() {
    std::vector<std::string> images;
    if (!fs::exists(kEyeImagesDir)) return images;
    for (const auto& entry : fs::directory_iterator(kEyeImagesDir)) {
        if (entry.path().filename().string().starts_with("real_")) {
            images.push_back(entry.path().string());
        }
    }
    std::sort(images.begin(), images.end());
    return images;
}

TEST(EyeFixtures, synthetic_eye_images_exist) {
    auto images = get_synthetic_images();
    ASSERT_GE(images.size(), 3u)
        << "Need at least 3 synthetic eye images in " << kEyeImagesDir;
    for (const auto& path : images) {
        EXPECT_TRUE(fs::exists(path)) << "Missing: " << path;
    }
}

TEST(EyeFixtures, real_eye_images_exist) {
    auto images = get_real_images();
    ASSERT_GE(images.size(), 2u)
        << "Need at least 2 real cropped eye images in " << kEyeImagesDir;
    for (const auto& path : images) {
        EXPECT_TRUE(fs::exists(path)) << "Missing: " << path;
    }
}

TEST(EyeFixtures, images_loadable_by_opencv) {
    auto synthetic = get_synthetic_images();
    ASSERT_FALSE(synthetic.empty());
    for (const auto& path : synthetic) {
        cv::Mat img = cv::imread(path, cv::IMREAD_UNCHANGED);
        EXPECT_FALSE(img.empty()) << "Failed to load: " << path;
    }

    auto real = get_real_images();
    ASSERT_FALSE(real.empty());
    for (const auto& path : real) {
        cv::Mat img = cv::imread(path, cv::IMREAD_UNCHANGED);
        EXPECT_FALSE(img.empty()) << "Failed to load: " << path;
    }
}

TEST(EyeFixtures, images_single_channel_or_three_channel) {
    auto synthetic = get_synthetic_images();
    for (const auto& path : synthetic) {
        cv::Mat img = cv::imread(path, cv::IMREAD_UNCHANGED);
        ASSERT_FALSE(img.empty()) << "Failed to load: " << path;
        EXPECT_TRUE(img.channels() == 1 || img.channels() == 3)
            << "Unexpected channels=" << img.channels() << " in " << path;
    }

    auto real = get_real_images();
    for (const auto& path : real) {
        cv::Mat img = cv::imread(path, cv::IMREAD_UNCHANGED);
        ASSERT_FALSE(img.empty()) << "Failed to load: " << path;
        EXPECT_TRUE(img.channels() == 1 || img.channels() == 3)
            << "Unexpected channels=" << img.channels() << " in " << path;
    }
}

TEST(EyeFixtures, synthetic_images_have_known_pupil) {
    // Filename convention: synthetic_pupil_CX_CY_R.png
    auto images = get_synthetic_images();
    ASSERT_FALSE(images.empty());

    for (const auto& path : images) {
        std::string filename = fs::path(path).stem().string();
        // Parse: synthetic_pupil_CX_CY_R
        ASSERT_TRUE(filename.starts_with("synthetic_pupil_"))
            << "Unexpected filename format: " << filename;

        std::string coords = filename.substr(std::string("synthetic_pupil_").size());
        // Split by '_' to get CX, CY, R
        size_t first = coords.find('_');
        size_t second = coords.find('_', first + 1);
        ASSERT_NE(first, std::string::npos) << "Missing CY in: " << filename;
        ASSERT_NE(second, std::string::npos) << "Missing R in: " << filename;

        int cx = std::stoi(coords.substr(0, first));
        int cy = std::stoi(coords.substr(first + 1, second - first - 1));
        int r = std::stoi(coords.substr(second + 1));

        EXPECT_GT(cx, 0) << "Invalid CX in " << filename;
        EXPECT_GT(cy, 0) << "Invalid CY in " << filename;
        EXPECT_GT(r, 0) << "Invalid R in " << filename;

        // Verify the pupil position is within the image bounds
        cv::Mat img = cv::imread(path, cv::IMREAD_UNCHANGED);
        ASSERT_FALSE(img.empty());
        EXPECT_LT(cx, img.cols) << "CX out of bounds in " << filename;
        EXPECT_LT(cy, img.rows) << "CY out of bounds in " << filename;
    }
}
