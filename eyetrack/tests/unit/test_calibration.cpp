#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include <eyetrack/core/node_registry.hpp>
#include <eyetrack/nodes/calibration_manager.hpp>
#include <eyetrack/utils/calibration_utils.hpp>

namespace fs = std::filesystem;

namespace {

// Helper: add N calibration points with linear mapping pupil = 0.3*screen + 0.1
void add_linear_points(eyetrack::CalibrationManager& mgr, int n) {
    for (int i = 0; i < n; ++i) {
        float sx = static_cast<float>(i % 3) / 2.0F;
        float sy = static_cast<float>(i / 3) / 2.0F;
        eyetrack::Point2D screen{sx, sy};
        eyetrack::PupilInfo pupil;
        pupil.center = {0.3F * sx + 0.1F, 0.3F * sy + 0.1F};
        pupil.radius = 10.0F;
        pupil.confidence = 0.9F;
        mgr.add_point(screen, pupil);
    }
}

std::string temp_yaml_path() {
    return (fs::temp_directory_path() / "eyetrack_test_calib.yaml").string();
}

}  // namespace

TEST(CalibrationManager, start_calibration_resets_state) {
    eyetrack::CalibrationManager mgr;
    mgr.start_calibration("user1");
    mgr.add_point({0.0F, 0.0F}, {{0.1F, 0.1F}, 10.0F, 0.9F});
    EXPECT_EQ(mgr.point_count(), 1U);

    mgr.start_calibration("user2");
    EXPECT_EQ(mgr.point_count(), 0U);
    EXPECT_EQ(mgr.profile().user_id, "user2");
}

TEST(CalibrationManager, add_point_increments_count) {
    eyetrack::CalibrationManager mgr;
    mgr.start_calibration("user1");

    mgr.add_point({0.0F, 0.0F}, {{0.1F, 0.1F}, 10.0F, 0.9F});
    mgr.add_point({0.5F, 0.5F}, {{0.2F, 0.2F}, 10.0F, 0.9F});
    mgr.add_point({1.0F, 1.0F}, {{0.4F, 0.4F}, 10.0F, 0.9F});
    EXPECT_EQ(mgr.point_count(), 3U);
}

TEST(CalibrationManager, finish_with_9_points_succeeds) {
    eyetrack::CalibrationManager mgr;
    mgr.start_calibration("user1");
    add_linear_points(mgr, 9);
    EXPECT_EQ(mgr.point_count(), 9U);

    auto result = mgr.finish_calibration();
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Transform should have valid matrices
    EXPECT_EQ(result->transform_left.rows(), 6);
    EXPECT_EQ(result->transform_left.cols(), 2);
}

TEST(CalibrationManager, finish_with_too_few_points_fails) {
    eyetrack::CalibrationManager mgr;
    mgr.start_calibration("user1");
    mgr.add_point({0.0F, 0.0F}, {{0.1F, 0.1F}, 10.0F, 0.9F});
    mgr.add_point({1.0F, 1.0F}, {{0.4F, 0.4F}, 10.0F, 0.9F});

    auto result = mgr.finish_calibration();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::CalibrationFailed);
}

TEST(CalibrationManager, save_profile_creates_file) {
    auto path = temp_yaml_path();

    eyetrack::CalibrationManager mgr;
    mgr.start_calibration("save_test");
    add_linear_points(mgr, 9);
    auto fit = mgr.finish_calibration();
    ASSERT_TRUE(fit.has_value()) << fit.error().message;

    auto save_result = mgr.save_profile(path);
    ASSERT_TRUE(save_result.has_value()) << save_result.error().message;

    EXPECT_TRUE(fs::exists(path));
    fs::remove(path);
}

TEST(CalibrationManager, load_profile_reads_file) {
    auto path = temp_yaml_path();

    // Save
    eyetrack::CalibrationManager saver;
    saver.start_calibration("load_test");
    add_linear_points(saver, 9);
    auto fit = saver.finish_calibration();
    ASSERT_TRUE(fit.has_value()) << fit.error().message;
    auto save_result = saver.save_profile(path);
    ASSERT_TRUE(save_result.has_value()) << save_result.error().message;

    // Load
    eyetrack::CalibrationManager loader;
    auto load_result = loader.load_profile(path);
    ASSERT_TRUE(load_result.has_value()) << load_result.error().message;

    EXPECT_EQ(loader.profile().user_id, "load_test");
    EXPECT_EQ(loader.profile().screen_points.size(), 9U);

    fs::remove(path);
}

TEST(CalibrationManager, save_load_round_trip) {
    auto path = temp_yaml_path();

    // Save
    eyetrack::CalibrationManager saver;
    saver.start_calibration("roundtrip");
    add_linear_points(saver, 9);
    auto fit = saver.finish_calibration();
    ASSERT_TRUE(fit.has_value()) << fit.error().message;
    auto save_result = saver.save_profile(path);
    ASSERT_TRUE(save_result.has_value()) << save_result.error().message;

    // Load
    eyetrack::CalibrationManager loader;
    auto load_result = loader.load_profile(path);
    ASSERT_TRUE(load_result.has_value()) << load_result.error().message;

    // Verify all fields preserved
    const auto& orig = saver.profile();
    const auto& loaded = loader.profile();
    EXPECT_EQ(orig.user_id, loaded.user_id);
    ASSERT_EQ(orig.screen_points.size(), loaded.screen_points.size());
    for (size_t i = 0; i < orig.screen_points.size(); ++i) {
        EXPECT_NEAR(orig.screen_points[i].x, loaded.screen_points[i].x, 1e-4F);
        EXPECT_NEAR(orig.screen_points[i].y, loaded.screen_points[i].y, 1e-4F);
    }
    ASSERT_EQ(orig.pupil_observations.size(),
              loaded.pupil_observations.size());
    for (size_t i = 0; i < orig.pupil_observations.size(); ++i) {
        EXPECT_NEAR(orig.pupil_observations[i].center.x,
                     loaded.pupil_observations[i].center.x, 1e-4F);
        EXPECT_NEAR(orig.pupil_observations[i].radius,
                     loaded.pupil_observations[i].radius, 1e-4F);
    }

    // Verify Eigen matrices preserved
    const auto& orig_t = saver.transform();
    const auto& loaded_t = loader.transform();
    ASSERT_EQ(orig_t.transform_left.rows(), loaded_t.transform_left.rows());
    ASSERT_EQ(orig_t.transform_left.cols(), loaded_t.transform_left.cols());
    for (int r = 0; r < orig_t.transform_left.rows(); ++r) {
        for (int c = 0; c < orig_t.transform_left.cols(); ++c) {
            EXPECT_NEAR(orig_t.transform_left(r, c),
                         loaded_t.transform_left(r, c), 1e-4F);
        }
    }

    fs::remove(path);
}

TEST(CalibrationManager, get_profile_by_user_id) {
    eyetrack::CalibrationManager mgr;
    mgr.start_calibration("user_abc");
    add_linear_points(mgr, 9);
    auto fit = mgr.finish_calibration();
    ASSERT_TRUE(fit.has_value()) << fit.error().message;

    auto profile = mgr.get_profile("user_abc");
    ASSERT_TRUE(profile.has_value()) << profile.error().message;
    EXPECT_EQ(profile->user_id, "user_abc");
    EXPECT_EQ(profile->screen_points.size(), 9U);
}

TEST(CalibrationManager, get_unknown_user_returns_error) {
    eyetrack::CalibrationManager mgr;

    auto profile = mgr.get_profile("nonexistent");
    ASSERT_FALSE(profile.has_value());
    EXPECT_EQ(profile.error().code, eyetrack::ErrorCode::CalibrationNotLoaded);
}

TEST(CalibrationManager, load_corrupted_file_returns_error) {
    auto path = temp_yaml_path();

    // Write garbled content
    {
        std::ofstream f(path);
        f << "{{{{not valid yaml: [[[";
    }

    eyetrack::CalibrationManager mgr;
    auto result = mgr.load_profile(path);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::ConfigInvalid);

    fs::remove(path);
}

TEST(CalibrationManager, node_registry_lookup) {
    auto& registry = eyetrack::NodeRegistry::instance();
    EXPECT_TRUE(registry.has("eyetrack.CalibrationManager"));

    auto factory = registry.lookup("eyetrack.CalibrationManager");
    ASSERT_TRUE(factory.has_value());

    eyetrack::NodeParams params;
    auto node = factory.value()(params);
    EXPECT_TRUE(node.has_value());
}

TEST(CalibrationManager, yaml_contains_eigen_matrices) {
    auto path = temp_yaml_path();

    eyetrack::CalibrationManager mgr;
    mgr.start_calibration("matrix_test");
    add_linear_points(mgr, 9);
    auto fit = mgr.finish_calibration();
    ASSERT_TRUE(fit.has_value()) << fit.error().message;
    auto save_result = mgr.save_profile(path);
    ASSERT_TRUE(save_result.has_value()) << save_result.error().message;

    // Read the YAML and verify matrix keys exist
    std::ifstream file(path);
    std::stringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();

    EXPECT_NE(content.find("transform_left"), std::string::npos)
        << "YAML should contain transform_left";
    EXPECT_NE(content.find("transform_right"), std::string::npos)
        << "YAML should contain transform_right";
    EXPECT_NE(content.find("rows"), std::string::npos)
        << "YAML should contain matrix rows";
    EXPECT_NE(content.find("cols"), std::string::npos)
        << "YAML should contain matrix cols";
    EXPECT_NE(content.find("data"), std::string::npos)
        << "YAML should contain matrix data (flat array)";

    fs::remove(path);
}
