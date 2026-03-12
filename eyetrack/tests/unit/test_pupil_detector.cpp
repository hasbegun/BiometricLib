#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <regex>
#include <string>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <eyetrack/core/node_registry.hpp>
#include <eyetrack/nodes/pupil_detector.hpp>

namespace fs = std::filesystem;

namespace {

std::string eye_fixtures_dir() {
    return std::string(EYETRACK_PROJECT_ROOT) + "/tests/fixtures/eye_images";
}

// Parse known pupil position from filename: synthetic_pupil_CX_CY_R.png
struct SyntheticInfo {
    float cx, cy, radius;
};

std::optional<SyntheticInfo> parse_synthetic_filename(const std::string& stem) {
    std::regex re(R"(synthetic_pupil_(\d+)_(\d+)_(\d+))");
    std::smatch match;
    if (std::regex_match(stem, match, re)) {
        return SyntheticInfo{
            std::stof(match[1].str()),
            std::stof(match[2].str()),
            std::stof(match[3].str())};
    }
    return std::nullopt;
}

// Load the first synthetic fixture that parses successfully.
struct SyntheticFixture {
    cv::Mat image;
    SyntheticInfo info;
};

std::optional<SyntheticFixture> load_first_synthetic() {
    auto dir = eye_fixtures_dir();
    if (!fs::exists(dir)) return std::nullopt;

    for (const auto& entry : fs::directory_iterator(dir)) {
        auto stem = entry.path().stem().string();
        auto info = parse_synthetic_filename(stem);
        if (!info) continue;
        cv::Mat img = cv::imread(entry.path().string(), cv::IMREAD_GRAYSCALE);
        if (img.empty()) continue;
        return SyntheticFixture{img, *info};
    }
    return std::nullopt;
}

// Load first real eye fixture.
std::optional<cv::Mat> load_first_real() {
    auto dir = eye_fixtures_dir();
    if (!fs::exists(dir)) return std::nullopt;

    for (const auto& entry : fs::directory_iterator(dir)) {
        auto name = entry.path().stem().string();
        if (name.find("real_") == 0) {
            cv::Mat img = cv::imread(entry.path().string(), cv::IMREAD_GRAYSCALE);
            if (!img.empty()) return img;
        }
    }
    return std::nullopt;
}

}  // namespace

// --- Centroid method tests ---

TEST(PupilDetector, centroid_detects_synthetic_pupil) {
    auto fixture = load_first_synthetic();
    ASSERT_TRUE(fixture.has_value()) << "No synthetic eye fixtures found";

    eyetrack::PupilDetector::Config cfg;
    cfg.method = eyetrack::PupilDetectorMethod::Centroid;
    eyetrack::PupilDetector detector(cfg);

    auto result = detector.run(fixture->image);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    float dx = result->center.x - fixture->info.cx;
    float dy = result->center.y - fixture->info.cy;
    float dist = std::sqrt(dx * dx + dy * dy);
    EXPECT_LE(dist, 3.0F) << "Center (" << result->center.x << ", "
                           << result->center.y << ") too far from expected ("
                           << fixture->info.cx << ", " << fixture->info.cy << ")";
}

TEST(PupilDetector, centroid_returns_valid_radius) {
    auto fixture = load_first_synthetic();
    ASSERT_TRUE(fixture.has_value()) << "No synthetic eye fixtures found";

    eyetrack::PupilDetector::Config cfg;
    cfg.method = eyetrack::PupilDetectorMethod::Centroid;
    eyetrack::PupilDetector detector(cfg);

    auto result = detector.run(fixture->image);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_GT(result->radius, 0.0F);
    // Radius should be in a reasonable range relative to the image
    EXPECT_LT(result->radius, static_cast<float>(
        std::max(fixture->image.cols, fixture->image.rows)));
}

TEST(PupilDetector, centroid_confidence_above_threshold) {
    auto fixture = load_first_synthetic();
    ASSERT_TRUE(fixture.has_value()) << "No synthetic eye fixtures found";

    eyetrack::PupilDetector::Config cfg;
    cfg.method = eyetrack::PupilDetectorMethod::Centroid;
    eyetrack::PupilDetector detector(cfg);

    auto result = detector.run(fixture->image);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_GT(result->confidence, 0.5F)
        << "Confidence " << result->confidence << " too low for clear synthetic pupil";
}

// --- Ellipse method tests ---

TEST(PupilDetector, ellipse_detects_synthetic_pupil) {
    auto fixture = load_first_synthetic();
    ASSERT_TRUE(fixture.has_value()) << "No synthetic eye fixtures found";

    eyetrack::PupilDetector::Config cfg;
    cfg.method = eyetrack::PupilDetectorMethod::Ellipse;
    eyetrack::PupilDetector detector(cfg);

    auto result = detector.run(fixture->image);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    float dx = result->center.x - fixture->info.cx;
    float dy = result->center.y - fixture->info.cy;
    float dist = std::sqrt(dx * dx + dy * dy);
    EXPECT_LE(dist, 5.0F) << "Center (" << result->center.x << ", "
                           << result->center.y << ") too far from expected ("
                           << fixture->info.cx << ", " << fixture->info.cy << ")";
}

TEST(PupilDetector, ellipse_returns_valid_axes) {
    auto fixture = load_first_synthetic();
    ASSERT_TRUE(fixture.has_value()) << "No synthetic eye fixtures found";

    eyetrack::PupilDetector::Config cfg;
    cfg.method = eyetrack::PupilDetectorMethod::Ellipse;
    eyetrack::PupilDetector detector(cfg);

    auto result = detector.run(fixture->image);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_GT(result->radius, 0.0F) << "Ellipse radius must be positive";
    EXPECT_LT(result->radius, static_cast<float>(
        std::max(fixture->image.cols, fixture->image.rows)));
}

// --- Config / method selection tests ---

TEST(PupilDetector, config_selects_centroid) {
    eyetrack::PupilDetector::Config cfg;
    cfg.method = eyetrack::PupilDetectorMethod::Centroid;
    eyetrack::PupilDetector detector(cfg);

    EXPECT_EQ(detector.method(), eyetrack::PupilDetectorMethod::Centroid);
}

TEST(PupilDetector, config_selects_ellipse) {
    eyetrack::PupilDetector::Config cfg;
    cfg.method = eyetrack::PupilDetectorMethod::Ellipse;
    eyetrack::PupilDetector detector(cfg);

    EXPECT_EQ(detector.method(), eyetrack::PupilDetectorMethod::Ellipse);
}

// --- Error cases ---

TEST(PupilDetector, empty_image_returns_error) {
    eyetrack::PupilDetector detector;
    cv::Mat empty;

    auto result = detector.run(empty);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::InvalidInput);
}

TEST(PupilDetector, uniform_white_image_returns_error) {
    eyetrack::PupilDetector detector;
    cv::Mat white(60, 120, CV_8UC1, cv::Scalar(255));

    auto result = detector.run(white);
    EXPECT_FALSE(result.has_value())
        << "Uniform white image should not detect a pupil";
}

// --- Real eye fixture test ---

TEST(PupilDetector, real_eye_image_detects_pupil) {
    auto real_img = load_first_real();
    if (!real_img.has_value()) {
        GTEST_SKIP() << "No real eye fixtures available";
    }

    eyetrack::PupilDetector detector;
    auto result = detector.run(*real_img);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_GT(result->radius, 0.0F);
    EXPECT_GT(result->confidence, 0.0F);
}

// --- Node registry ---

TEST(PupilDetector, node_registry_lookup) {
    auto& registry = eyetrack::NodeRegistry::instance();
    EXPECT_TRUE(registry.has("eyetrack.PupilDetector"));

    auto factory = registry.lookup("eyetrack.PupilDetector");
    ASSERT_TRUE(factory.has_value());

    eyetrack::NodeParams params;
    auto node = factory.value()(params);
    EXPECT_TRUE(node.has_value());
}
