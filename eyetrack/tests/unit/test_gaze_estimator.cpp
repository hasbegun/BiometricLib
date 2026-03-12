#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include <eyetrack/core/node_registry.hpp>
#include <eyetrack/nodes/gaze_estimator.hpp>
#include <eyetrack/utils/calibration_utils.hpp>

namespace {

// Build a calibration transform from a linear mapping: screen = pupil (identity)
eyetrack::CalibrationTransform make_identity_transform() {
    std::vector<eyetrack::Point2D> pupil = {
        {0.0F, 0.0F}, {1.0F, 0.0F}, {0.0F, 1.0F},
        {1.0F, 1.0F}, {0.5F, 0.5F}};
    std::vector<eyetrack::Point2D> screen = pupil;

    auto coeffs = eyetrack::least_squares_fit(pupil, screen);
    eyetrack::CalibrationTransform t;
    t.transform_left = *coeffs;
    t.transform_right = *coeffs;
    return t;
}

// Build a linear transform: screen = 2 * pupil
eyetrack::CalibrationTransform make_linear_transform() {
    std::vector<eyetrack::Point2D> pupil = {
        {0.0F, 0.0F}, {0.5F, 0.0F}, {0.0F, 0.5F},
        {0.5F, 0.5F}, {0.25F, 0.25F}, {0.25F, 0.5F}};
    std::vector<eyetrack::Point2D> screen;
    for (const auto& p : pupil) {
        screen.push_back({2.0F * p.x, 2.0F * p.y});
    }

    auto coeffs = eyetrack::least_squares_fit(pupil, screen);
    eyetrack::CalibrationTransform t;
    t.transform_left = *coeffs;
    t.transform_right = *coeffs;
    return t;
}

}  // namespace

TEST(GazeEstimator, synthetic_linear_calibration) {
    auto transform = make_linear_transform();
    eyetrack::GazeEstimator estimator;
    estimator.set_calibration(transform);

    eyetrack::PupilInfo pupil;
    pupil.center = {0.25F, 0.25F};
    pupil.confidence = 0.9F;

    auto result = estimator.estimate(pupil);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // screen = 2 * pupil = (0.5, 0.5)
    // 2 degrees out of 90 degrees FOV ≈ 0.022 normalized
    EXPECT_NEAR(result->gaze_point.x, 0.5F, 0.05F);
    EXPECT_NEAR(result->gaze_point.y, 0.5F, 0.05F);
}

TEST(GazeEstimator, output_normalized_0_to_1) {
    auto transform = make_identity_transform();
    eyetrack::GazeEstimator estimator;
    estimator.set_calibration(transform);

    // Test with values that map within [0,1]
    eyetrack::PupilInfo pupil;
    pupil.center = {0.7F, 0.3F};
    pupil.confidence = 0.8F;

    auto result = estimator.estimate(pupil);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_GE(result->gaze_point.x, 0.0F);
    EXPECT_LE(result->gaze_point.x, 1.0F);
    EXPECT_GE(result->gaze_point.y, 0.0F);
    EXPECT_LE(result->gaze_point.y, 1.0F);

    // Test with values that would extrapolate beyond [0,1] — should be clamped
    pupil.center = {5.0F, 5.0F};
    result = estimator.estimate(pupil);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_LE(result->gaze_point.x, 1.0F);
    EXPECT_LE(result->gaze_point.y, 1.0F);
}

TEST(GazeEstimator, uses_polynomial_features) {
    // Verify that the polynomial mapping produces correct results
    // by checking that identity transform maps (0.5, 0.5) -> ~(0.5, 0.5)
    auto transform = make_identity_transform();
    eyetrack::GazeEstimator estimator;
    estimator.set_calibration(transform);

    eyetrack::PupilInfo pupil;
    pupil.center = {0.5F, 0.5F};
    pupil.confidence = 1.0F;

    auto result = estimator.estimate(pupil);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_NEAR(result->gaze_point.x, 0.5F, 0.01F);
    EXPECT_NEAR(result->gaze_point.y, 0.5F, 0.01F);
}

TEST(GazeEstimator, no_calibration_returns_error) {
    eyetrack::GazeEstimator estimator;
    EXPECT_FALSE(estimator.is_calibrated());

    eyetrack::PupilInfo pupil;
    pupil.center = {0.5F, 0.5F};

    auto result = estimator.estimate(pupil);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::CalibrationNotLoaded);
}

TEST(GazeEstimator, confidence_output) {
    auto transform = make_identity_transform();
    eyetrack::GazeEstimator estimator;
    estimator.set_calibration(transform);

    eyetrack::PupilInfo pupil;
    pupil.center = {0.5F, 0.5F};
    pupil.confidence = 0.85F;

    auto result = estimator.estimate(pupil);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_GE(result->confidence, 0.0F);
    EXPECT_LE(result->confidence, 1.0F);
    EXPECT_FLOAT_EQ(result->confidence, 0.85F);
}

TEST(GazeEstimator, node_registry_lookup) {
    auto& registry = eyetrack::NodeRegistry::instance();
    EXPECT_TRUE(registry.has("eyetrack.GazeEstimator"));

    auto factory = registry.lookup("eyetrack.GazeEstimator");
    ASSERT_TRUE(factory.has_value());

    eyetrack::NodeParams params;
    auto node = factory.value()(params);
    EXPECT_TRUE(node.has_value());
}
