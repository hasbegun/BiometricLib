#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include <eyetrack/core/node_registry.hpp>
#include <eyetrack/nodes/gaze_smoother.hpp>

TEST(GazeSmoother, no_smoothing_first_frame) {
    eyetrack::GazeSmoother smoother;

    eyetrack::GazeResult input;
    input.gaze_point = {0.7F, 0.3F};
    input.confidence = 0.9F;

    auto output = smoother.smooth(input);
    EXPECT_FLOAT_EQ(output.gaze_point.x, 0.7F);
    EXPECT_FLOAT_EQ(output.gaze_point.y, 0.3F);
}

TEST(GazeSmoother, ema_converges_to_constant) {
    eyetrack::GazeSmoother smoother;

    eyetrack::GazeResult input;
    input.gaze_point = {0.5F, 0.5F};
    input.confidence = 1.0F;

    // Feed the same value many times
    eyetrack::GazeResult output;
    for (int i = 0; i < 100; ++i) {
        output = smoother.smooth(input);
    }

    EXPECT_NEAR(output.gaze_point.x, 0.5F, 0.001F);
    EXPECT_NEAR(output.gaze_point.y, 0.5F, 0.001F);
}

TEST(GazeSmoother, ema_smooths_jitter) {
    eyetrack::GazeSmoother smoother;

    // Compute variance of jittery input vs smoothed output
    std::vector<float> raw_x, smooth_x;

    for (int i = 0; i < 50; ++i) {
        eyetrack::GazeResult input;
        // Alternate between 0.4 and 0.6 (jittery)
        float jitter = (i % 2 == 0) ? 0.4F : 0.6F;
        input.gaze_point = {jitter, 0.5F};
        input.confidence = 1.0F;

        raw_x.push_back(jitter);
        auto output = smoother.smooth(input);
        smooth_x.push_back(output.gaze_point.x);
    }

    // Compute variance of last 20 values
    auto variance = [](const std::vector<float>& v, size_t start) {
        float mean = 0.0F;
        size_t n = v.size() - start;
        for (size_t i = start; i < v.size(); ++i) mean += v[i];
        mean /= static_cast<float>(n);
        float var = 0.0F;
        for (size_t i = start; i < v.size(); ++i) {
            float d = v[i] - mean;
            var += d * d;
        }
        return var / static_cast<float>(n);
    };

    float raw_var = variance(raw_x, 30);
    float smooth_var = variance(smooth_x, 30);
    EXPECT_LT(smooth_var, raw_var)
        << "Smoothed output should have lower variance than raw input";
}

TEST(GazeSmoother, alpha_0_no_update) {
    eyetrack::GazeSmoother::Config cfg;
    cfg.alpha = 0.0F;
    eyetrack::GazeSmoother smoother(cfg);

    eyetrack::GazeResult first;
    first.gaze_point = {0.3F, 0.3F};
    auto out1 = smoother.smooth(first);
    EXPECT_FLOAT_EQ(out1.gaze_point.x, 0.3F);  // first frame passthrough

    eyetrack::GazeResult second;
    second.gaze_point = {0.9F, 0.9F};
    auto out2 = smoother.smooth(second);
    // alpha=0 means: smoothed = 0*current + 1*previous = previous
    EXPECT_FLOAT_EQ(out2.gaze_point.x, 0.3F);
    EXPECT_FLOAT_EQ(out2.gaze_point.y, 0.3F);
}

TEST(GazeSmoother, alpha_1_no_smoothing) {
    eyetrack::GazeSmoother::Config cfg;
    cfg.alpha = 1.0F;
    eyetrack::GazeSmoother smoother(cfg);

    eyetrack::GazeResult first;
    first.gaze_point = {0.3F, 0.3F};
    (void)smoother.smooth(first);

    eyetrack::GazeResult second;
    second.gaze_point = {0.8F, 0.7F};
    auto out2 = smoother.smooth(second);
    // alpha=1 means: smoothed = 1*current + 0*previous = current
    EXPECT_FLOAT_EQ(out2.gaze_point.x, 0.8F);
    EXPECT_FLOAT_EQ(out2.gaze_point.y, 0.7F);
}

TEST(GazeSmoother, configurable_alpha) {
    eyetrack::GazeSmoother::Config cfg;
    cfg.alpha = 0.5F;
    eyetrack::GazeSmoother smoother(cfg);
    EXPECT_FLOAT_EQ(smoother.alpha(), 0.5F);

    eyetrack::GazeResult first;
    first.gaze_point = {0.0F, 0.0F};
    (void)smoother.smooth(first);

    eyetrack::GazeResult second;
    second.gaze_point = {1.0F, 1.0F};
    auto out = smoother.smooth(second);
    // smoothed = 0.5 * 1.0 + 0.5 * 0.0 = 0.5
    EXPECT_FLOAT_EQ(out.gaze_point.x, 0.5F);
    EXPECT_FLOAT_EQ(out.gaze_point.y, 0.5F);
}

TEST(GazeSmoother, node_registry_lookup) {
    auto& registry = eyetrack::NodeRegistry::instance();
    EXPECT_TRUE(registry.has("eyetrack.GazeSmoother"));

    auto factory = registry.lookup("eyetrack.GazeSmoother");
    ASSERT_TRUE(factory.has_value());

    eyetrack::NodeParams params;
    auto node = factory.value()(params);
    EXPECT_TRUE(node.has_value());
}
