#include <gtest/gtest.h>

#include <chrono>

#include <eyetrack/core/node_registry.hpp>
#include <eyetrack/nodes/blink_detector.hpp>

using eyetrack::BlinkDetector;
using eyetrack::BlinkState;
using eyetrack::BlinkType;
using Clock = std::chrono::steady_clock;

namespace {

Clock::time_point make_tp(int ms) {
    return Clock::time_point{} + std::chrono::milliseconds(ms);
}

}  // namespace

TEST(BlinkDetector, initial_state_is_open) {
    BlinkDetector detector;
    EXPECT_EQ(detector.state(), BlinkState::Open);
}

TEST(BlinkDetector, ear_below_threshold_transitions_to_closing) {
    BlinkDetector detector;

    // Feed EAR below default threshold (0.21)
    auto result = detector.update(0.15F, make_tp(0));
    EXPECT_EQ(result, BlinkType::None);
    EXPECT_EQ(detector.state(), BlinkState::Closing);
}

TEST(BlinkDetector, ear_above_threshold_returns_to_open) {
    BlinkDetector detector;

    // Go to Closing
    (void)detector.update(0.15F, make_tp(0));
    EXPECT_EQ(detector.state(), BlinkState::Closing);

    // Reopen after <100ms (noise) — should go back to Open
    auto result = detector.update(0.30F, make_tp(50));
    EXPECT_EQ(result, BlinkType::None);
    EXPECT_EQ(detector.state(), BlinkState::Open);
}

TEST(BlinkDetector, configurable_ear_threshold) {
    BlinkDetector::Config cfg;
    cfg.ear_threshold = 0.18F;
    BlinkDetector detector(cfg);

    // EAR of 0.19 is above 0.18 threshold — should stay Open
    (void)detector.update(0.19F, make_tp(0));
    EXPECT_EQ(detector.state(), BlinkState::Open);

    // EAR of 0.17 is below 0.18 threshold — should go to Closing
    auto result2 = detector.update(0.17F, make_tp(10));
    EXPECT_EQ(result2, BlinkType::None);
    EXPECT_EQ(detector.state(), BlinkState::Closing);
}

TEST(BlinkDetector, configurable_timing_params) {
    BlinkDetector::Config cfg;
    cfg.min_blink_ms = 50;
    cfg.max_single_ms = 300;
    cfg.long_threshold_ms = 600;
    cfg.double_window_ms = 400;
    BlinkDetector detector(cfg);

    EXPECT_EQ(detector.config().min_blink_ms, 50);
    EXPECT_EQ(detector.config().max_single_ms, 300);
    EXPECT_EQ(detector.config().long_threshold_ms, 600);
    EXPECT_EQ(detector.config().double_window_ms, 400);

    // Verify timing: 60ms blink (> min_blink=50, < max_single=300) -> single
    (void)detector.update(0.10F, make_tp(0));     // close
    EXPECT_EQ(detector.state(), BlinkState::Closing);

    // Reopen after 60ms -> enters WaitingDouble since 60 >= 50
    auto result = detector.update(0.30F, make_tp(60));
    EXPECT_EQ(detector.state(), BlinkState::WaitingDouble);

    // Wait for double_window timeout (400ms)
    result = detector.update(0.30F, make_tp(500));
    EXPECT_EQ(result, BlinkType::Single);
    EXPECT_EQ(detector.state(), BlinkState::Open);
}

TEST(BlinkDetector, per_user_baseline) {
    BlinkDetector detector;

    // Without baseline, threshold = 0.21 (default)
    EXPECT_FLOAT_EQ(detector.effective_threshold(), 0.21F);

    // Set baseline of 0.30 (open eye EAR)
    // Effective threshold = 0.30 * 0.7 = 0.21
    detector.set_baseline(0.30F);
    EXPECT_NEAR(detector.effective_threshold(), 0.21F, 0.001F);

    // Set higher baseline -> higher threshold
    detector.set_baseline(0.40F);
    EXPECT_NEAR(detector.effective_threshold(), 0.28F, 0.001F);

    // Verify that EAR=0.25 is below new threshold (0.28)
    detector.reset();
    (void)detector.update(0.25F, make_tp(0));
    EXPECT_EQ(detector.state(), BlinkState::Closing);
}

TEST(BlinkDetector, node_registry_lookup) {
    auto& registry = eyetrack::NodeRegistry::instance();
    EXPECT_TRUE(registry.has("eyetrack.BlinkDetector"));

    auto factory = registry.lookup("eyetrack.BlinkDetector");
    ASSERT_TRUE(factory.has_value());

    eyetrack::NodeParams params;
    auto node = factory.value()(params);
    EXPECT_TRUE(node.has_value());
}
