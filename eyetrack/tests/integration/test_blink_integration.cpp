#include <gtest/gtest.h>

#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <eyetrack/core/types.hpp>
#include <eyetrack/nodes/blink_detector.hpp>
#include <eyetrack/utils/geometry_utils.hpp>

using eyetrack::BlinkDetector;
using eyetrack::BlinkType;
using eyetrack::EyeLandmarks;
using eyetrack::Point2D;
using eyetrack::compute_ear;
using eyetrack::compute_ear_average;
using Clock = std::chrono::steady_clock;

namespace {

Clock::time_point make_tp(int ms) {
    return Clock::time_point{} + std::chrono::milliseconds(ms);
}

// Build a 6-point eye with given vertical opening (same helper as test_ear_computation).
std::array<Point2D, 6> make_eye(float width, float half_height) {
    float hw = width / 2.0F;
    return {{
        {-hw, 0.0F},
        {-hw / 2.0F, half_height},
        {hw / 2.0F, half_height},
        {hw, 0.0F},
        {hw / 2.0F, -half_height},
        {-hw / 2.0F, -half_height},
    }};
}

// Build EyeLandmarks that produce a given EAR value.
// EAR = (v1+v2)/(2*h) = (2*hh + 2*hh)/(2*width) = 2*hh/width
// So half_height = ear * width / 2
EyeLandmarks make_landmarks_for_ear(float ear) {
    float half_height = ear * 30.0F / 2.0F;
    EyeLandmarks lm;
    lm.left = make_eye(30.0F, half_height);
    lm.right = make_eye(30.0F, half_height);
    return lm;
}

struct EarSample {
    int timestamp_ms;
    float ear;
};

std::vector<EarSample> load_ear_fixture(const std::string& filename) {
    std::string path = std::string(EYETRACK_PROJECT_ROOT) +
                       "/tests/fixtures/ear_sequences/" + filename;
    std::ifstream file(path);
    std::vector<EarSample> samples;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string ts_str;
        std::string ear_str;
        if (std::getline(iss, ts_str, ',') && std::getline(iss, ear_str, ',')) {
            samples.push_back({std::stoi(ts_str), std::stof(ear_str)});
        }
    }
    return samples;
}

// Run a sequence through the detector and collect all blink events.
std::vector<BlinkType> run_sequence(BlinkDetector& detector,
                                    const std::vector<EarSample>& samples) {
    std::vector<BlinkType> events;
    for (const auto& s : samples) {
        auto result = detector.update(s.ear, make_tp(s.timestamp_ms));
        if (result != BlinkType::None) {
            events.push_back(result);
        }
    }
    return events;
}

}  // namespace

// End-to-end: EyeLandmarks -> compute_ear_average -> BlinkDetector
TEST(BlinkIntegration, landmarks_to_ear_to_blink) {
    BlinkDetector detector;

    // Open eyes (EAR ~0.3)
    auto open_lm = make_landmarks_for_ear(0.30F);
    float open_ear = compute_ear_average(open_lm);
    EXPECT_NEAR(open_ear, 0.30F, 0.01F);
    (void)detector.update(open_ear, make_tp(0));

    // Close eyes (EAR ~0.08)
    auto closed_lm = make_landmarks_for_ear(0.08F);
    float closed_ear = compute_ear_average(closed_lm);
    EXPECT_NEAR(closed_ear, 0.08F, 0.01F);
    (void)detector.update(closed_ear, make_tp(100));

    // Still closed at 200ms
    (void)detector.update(closed_ear, make_tp(200));

    // Reopen at 300ms (200ms duration -> WaitingDouble)
    auto result = detector.update(open_ear, make_tp(300));
    EXPECT_EQ(result, BlinkType::None);  // Waiting for potential double

    // Timeout -> Single
    result = detector.update(open_ear, make_tp(900));
    EXPECT_EQ(result, BlinkType::Single);
}

// Fixture-based tests
TEST(BlinkIntegration, fixture_ear_sequence_single) {
    auto samples = load_ear_fixture("single_blink.csv");
    ASSERT_FALSE(samples.empty()) << "Failed to load single_blink.csv fixture";

    BlinkDetector detector;
    auto events = run_sequence(detector, samples);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0], BlinkType::Single);
}

TEST(BlinkIntegration, fixture_ear_sequence_double) {
    auto samples = load_ear_fixture("double_blink.csv");
    ASSERT_FALSE(samples.empty()) << "Failed to load double_blink.csv fixture";

    BlinkDetector detector;
    auto events = run_sequence(detector, samples);

    // Should contain at least one Double event
    bool has_double = false;
    for (auto e : events) {
        if (e == BlinkType::Double) has_double = true;
    }
    EXPECT_TRUE(has_double) << "Expected Double blink in sequence";
}

TEST(BlinkIntegration, fixture_ear_sequence_long) {
    auto samples = load_ear_fixture("long_blink.csv");
    ASSERT_FALSE(samples.empty()) << "Failed to load long_blink.csv fixture";

    BlinkDetector detector;
    auto events = run_sequence(detector, samples);

    // Should contain at least one Long event
    bool has_long = false;
    for (auto e : events) {
        if (e == BlinkType::Long) has_long = true;
    }
    EXPECT_TRUE(has_long) << "Expected Long blink in sequence";
}

// Performance: blink detection should be very fast (<0.5ms per frame)
TEST(BlinkIntegration, blink_detection_performance) {
    BlinkDetector detector;

    constexpr int kFrames = 10000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < kFrames; ++i) {
        // Simulate alternating open/close pattern
        float ear = (i % 20 < 5) ? 0.10F : 0.30F;
        (void)detector.update(ear, make_tp(i * 33));
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto total_us = std::chrono::duration_cast<std::chrono::microseconds>(
                        end - start)
                        .count();
    double per_frame_ms = static_cast<double>(total_us) /
                          static_cast<double>(kFrames) / 1000.0;

    EXPECT_LT(per_frame_ms, 0.5)
        << "Blink detection took " << per_frame_ms << "ms per frame";
}
