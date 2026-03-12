#include <gtest/gtest.h>

#include <chrono>

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

// --- Behavior tests for blink classification ---

TEST(BlinkStateMachine, constant_high_ear_no_blinks) {
    BlinkDetector detector;

    // Feed 100 frames of EAR=0.3 at 30fps (~33ms per frame)
    for (int i = 0; i < 100; ++i) {
        auto result = detector.update(0.30F, make_tp(i * 33));
        EXPECT_EQ(result, BlinkType::None)
            << "Frame " << i << " should not produce a blink";
    }
    EXPECT_EQ(detector.state(), BlinkState::Open);
}

TEST(BlinkStateMachine, single_blink_200ms) {
    BlinkDetector detector;

    // Start open
    (void)detector.update(0.30F, make_tp(0));

    // Close for 200ms (>100ms min_blink, <=400ms max_single)
    (void)detector.update(0.10F, make_tp(100));
    EXPECT_EQ(detector.state(), BlinkState::Closing);

    // Reopen at 300ms (200ms duration)
    auto result = detector.update(0.30F, make_tp(300));
    EXPECT_EQ(result, BlinkType::None);  // Goes to WaitingDouble, not emitted yet
    EXPECT_EQ(detector.state(), BlinkState::WaitingDouble);

    // Wait for double_window timeout (500ms from reopen)
    result = detector.update(0.30F, make_tp(850));
    EXPECT_EQ(result, BlinkType::Single);
    EXPECT_EQ(detector.state(), BlinkState::Open);
}

TEST(BlinkStateMachine, double_blink_within_500ms) {
    BlinkDetector detector;

    // First blink: close at 0ms, reopen at 200ms (200ms duration)
    (void)detector.update(0.30F, make_tp(0));
    (void)detector.update(0.10F, make_tp(100));
    (void)detector.update(0.30F, make_tp(300));  // -> WaitingDouble
    EXPECT_EQ(detector.state(), BlinkState::WaitingDouble);

    // Second blink starts within 500ms of first reopen
    auto result = detector.update(0.10F, make_tp(500));
    EXPECT_EQ(result, BlinkType::Double);
    EXPECT_EQ(detector.state(), BlinkState::Closing);  // Tracking the second blink

    // Second blink reopens
    result = detector.update(0.30F, make_tp(700));
    // Duration 200ms >= min_blink, goes to WaitingDouble again
    EXPECT_EQ(detector.state(), BlinkState::WaitingDouble);
}

TEST(BlinkStateMachine, long_blink_800ms) {
    BlinkDetector detector;

    // Start open
    (void)detector.update(0.30F, make_tp(0));

    // Close at 100ms
    (void)detector.update(0.10F, make_tp(100));
    EXPECT_EQ(detector.state(), BlinkState::Closing);

    // Still closed at 500ms — not yet long_threshold (500ms from close_start at 100ms)
    auto result = detector.update(0.10F, make_tp(500));
    EXPECT_EQ(result, BlinkType::None);

    // Still closed at 900ms — 800ms duration >= long_threshold (500ms)
    result = detector.update(0.10F, make_tp(900));
    EXPECT_EQ(result, BlinkType::Long);
    EXPECT_EQ(detector.state(), BlinkState::Open);
}

TEST(BlinkStateMachine, noise_50ms_ignored) {
    BlinkDetector detector;

    // Start open
    (void)detector.update(0.30F, make_tp(0));

    // Brief close for 50ms (< min_blink=100ms)
    (void)detector.update(0.10F, make_tp(100));
    EXPECT_EQ(detector.state(), BlinkState::Closing);

    // Reopen after only 50ms — should be noise
    auto result = detector.update(0.30F, make_tp(150));
    EXPECT_EQ(result, BlinkType::None);
    EXPECT_EQ(detector.state(), BlinkState::Open);
}

TEST(BlinkStateMachine, alternating_patterns_classified) {
    BlinkDetector detector;

    // Pattern: noise (50ms) -> single (200ms) -> noise (30ms) -> long (600ms)
    (void)detector.update(0.30F, make_tp(0));

    // Noise: 50ms blink
    (void)detector.update(0.10F, make_tp(100));
    auto r1 = detector.update(0.30F, make_tp(150));
    EXPECT_EQ(r1, BlinkType::None);  // noise filtered
    EXPECT_EQ(detector.state(), BlinkState::Open);

    // Single: 200ms blink
    (void)detector.update(0.10F, make_tp(500));
    auto r2 = detector.update(0.30F, make_tp(700));
    EXPECT_EQ(r2, BlinkType::None);  // enters WaitingDouble
    EXPECT_EQ(detector.state(), BlinkState::WaitingDouble);

    // Let it timeout -> Single
    auto r3 = detector.update(0.30F, make_tp(1300));
    EXPECT_EQ(r3, BlinkType::Single);
    EXPECT_EQ(detector.state(), BlinkState::Open);

    // Long: 600ms blink (detected while still closing)
    (void)detector.update(0.10F, make_tp(1500));
    (void)detector.update(0.10F, make_tp(1900));  // 400ms, not yet long
    auto r4 = detector.update(0.10F, make_tp(2200));  // 700ms >= 500ms
    EXPECT_EQ(r4, BlinkType::Long);
    EXPECT_EQ(detector.state(), BlinkState::Open);
}

TEST(BlinkStateMachine, custom_threshold_changes_detection) {
    BlinkDetector::Config cfg;
    cfg.ear_threshold = 0.15F;
    BlinkDetector detector(cfg);

    // EAR=0.18 is above 0.15 threshold — should NOT trigger
    (void)detector.update(0.18F, make_tp(0));
    EXPECT_EQ(detector.state(), BlinkState::Open);

    // EAR=0.14 is below 0.15 threshold — should trigger
    (void)detector.update(0.14F, make_tp(100));
    EXPECT_EQ(detector.state(), BlinkState::Closing);
}

TEST(BlinkStateMachine, rapid_triple_blink_detected_as_two_events) {
    BlinkDetector detector;

    // First blink: 150ms
    (void)detector.update(0.30F, make_tp(0));
    (void)detector.update(0.10F, make_tp(100));
    (void)detector.update(0.30F, make_tp(250));  // -> WaitingDouble
    EXPECT_EQ(detector.state(), BlinkState::WaitingDouble);

    // Second blink: starts 100ms after first reopen (within 500ms window)
    auto r1 = detector.update(0.10F, make_tp(350));
    EXPECT_EQ(r1, BlinkType::Double);  // Double detected
    EXPECT_EQ(detector.state(), BlinkState::Closing);

    // Second blink ends at 500ms (150ms duration)
    (void)detector.update(0.30F, make_tp(500));
    EXPECT_EQ(detector.state(), BlinkState::WaitingDouble);

    // Third blink: starts 100ms after second reopen (within 500ms window)
    auto r3 = detector.update(0.10F, make_tp(600));
    EXPECT_EQ(r3, BlinkType::Double);  // Another Double from second+third

    // Third blink ends
    (void)detector.update(0.30F, make_tp(750));

    // Wait for timeout
    auto r4 = detector.update(0.30F, make_tp(1300));
    EXPECT_EQ(r4, BlinkType::Single);  // Pending single from third blink
}

TEST(BlinkStateMachine, waiting_double_timeout_confirms_single) {
    BlinkDetector detector;

    // Single blink: 150ms
    (void)detector.update(0.30F, make_tp(0));
    (void)detector.update(0.10F, make_tp(100));
    (void)detector.update(0.30F, make_tp(250));  // -> WaitingDouble
    EXPECT_EQ(detector.state(), BlinkState::WaitingDouble);

    // Feed frames during the waiting window — no blink event
    auto r1 = detector.update(0.30F, make_tp(400));
    EXPECT_EQ(r1, BlinkType::None);
    EXPECT_EQ(detector.state(), BlinkState::WaitingDouble);

    auto r2 = detector.update(0.30F, make_tp(600));
    EXPECT_EQ(r2, BlinkType::None);
    EXPECT_EQ(detector.state(), BlinkState::WaitingDouble);

    // Timeout at >500ms after reopen (250ms + 500ms = 750ms)
    auto r3 = detector.update(0.30F, make_tp(800));
    EXPECT_EQ(r3, BlinkType::Single);
    EXPECT_EQ(detector.state(), BlinkState::Open);
}

TEST(BlinkStateMachine, state_resets_after_long_blink) {
    BlinkDetector detector;

    // Long blink: eyes closed for 600ms
    (void)detector.update(0.30F, make_tp(0));
    (void)detector.update(0.10F, make_tp(100));

    // Long detected while still closing
    auto result = detector.update(0.10F, make_tp(700));
    EXPECT_EQ(result, BlinkType::Long);
    EXPECT_EQ(detector.state(), BlinkState::Open);

    // After long blink, detector should work normally for next blink
    (void)detector.update(0.10F, make_tp(1000));
    EXPECT_EQ(detector.state(), BlinkState::Closing);

    (void)detector.update(0.30F, make_tp(1200));  // 200ms -> WaitingDouble
    EXPECT_EQ(detector.state(), BlinkState::WaitingDouble);

    auto r2 = detector.update(0.30F, make_tp(1800));
    EXPECT_EQ(r2, BlinkType::Single);
    EXPECT_EQ(detector.state(), BlinkState::Open);
}

TEST(BlinkStateMachine, boundary_duration_100ms) {
    BlinkDetector detector;

    // Exactly 100ms = min_blink_ms -> should count as Single (not noise)
    (void)detector.update(0.30F, make_tp(0));
    (void)detector.update(0.10F, make_tp(100));

    auto result = detector.update(0.30F, make_tp(200));  // exactly 100ms
    // 100ms >= min_blink(100) and <= max_single(400) -> WaitingDouble
    EXPECT_EQ(result, BlinkType::None);
    EXPECT_EQ(detector.state(), BlinkState::WaitingDouble);

    // Confirm as single after timeout
    auto r2 = detector.update(0.30F, make_tp(750));
    EXPECT_EQ(r2, BlinkType::Single);
}

TEST(BlinkStateMachine, boundary_duration_400ms) {
    BlinkDetector detector;

    // Exactly 400ms = max_single_ms -> should count as Single
    (void)detector.update(0.30F, make_tp(0));
    (void)detector.update(0.10F, make_tp(100));

    auto result = detector.update(0.30F, make_tp(500));  // exactly 400ms
    // 400ms >= min_blink(100) and <= max_single(400) -> WaitingDouble
    EXPECT_EQ(result, BlinkType::None);
    EXPECT_EQ(detector.state(), BlinkState::WaitingDouble);

    // Confirm as single after timeout
    auto r2 = detector.update(0.30F, make_tp(1100));
    EXPECT_EQ(r2, BlinkType::Single);
}

TEST(BlinkStateMachine, boundary_duration_500ms) {
    BlinkDetector detector;

    // Exactly 500ms = long_threshold_ms -> should be Long
    (void)detector.update(0.30F, make_tp(0));
    (void)detector.update(0.10F, make_tp(100));

    // At 600ms, duration is exactly 500ms >= long_threshold
    auto result = detector.update(0.30F, make_tp(600));
    EXPECT_EQ(result, BlinkType::Long);
    EXPECT_EQ(detector.state(), BlinkState::Open);
}
