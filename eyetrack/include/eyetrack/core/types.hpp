#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

namespace eyetrack {

// ---- Basic geometry ----

struct Point2D {
    float x = 0.0F;
    float y = 0.0F;

    bool operator==(const Point2D& other) const = default;
};

std::ostream& operator<<(std::ostream& os, const Point2D& p);

// ---- Eye landmarks (6-point EAR subset per eye) ----

struct EyeLandmarks {
    std::array<Point2D, 6> left{};
    std::array<Point2D, 6> right{};

    bool operator==(const EyeLandmarks& other) const = default;
};

std::ostream& operator<<(std::ostream& os, const EyeLandmarks& lm);

// ---- Pupil detection result ----

struct PupilInfo {
    Point2D center{};
    float radius = 0.0F;
    float confidence = 0.0F;

    bool operator==(const PupilInfo& other) const = default;
};

std::ostream& operator<<(std::ostream& os, const PupilInfo& pi);

// ---- Blink classification ----

enum class BlinkType : uint8_t {
    None = 0,
    Single = 1,
    Double = 2,
    Long = 3,
};

std::ostream& operator<<(std::ostream& os, BlinkType bt);

// ---- Gaze estimation result ----

struct GazeResult {
    Point2D gaze_point{};
    float confidence = 0.0F;
    BlinkType blink = BlinkType::None;
    std::chrono::steady_clock::time_point timestamp{};

    bool operator==(const GazeResult& other) const = default;
};

std::ostream& operator<<(std::ostream& os, const GazeResult& gr);

// ---- Face detection output ----

struct FaceROI {
    cv::Rect2f bounding_box{};
    std::vector<Point2D> landmarks;  // 468 MediaPipe FaceMesh landmarks
    float confidence = 0.0F;
};

std::ostream& operator<<(std::ostream& os, const FaceROI& roi);

// ---- Per-frame input data ----

struct FrameData {
    cv::Mat frame;  // CV_8UC3 BGR or CV_8UC1 grayscale
    uint64_t frame_id = 0;
    std::chrono::steady_clock::time_point timestamp{};
    std::string client_id;
};

std::ostream& operator<<(std::ostream& os, const FrameData& fd);

// ---- Calibration profile (NO Eigen dependency) ----
// Eigen-based transform matrices belong in calibration_utils (Phase 3).

struct CalibrationProfile {
    std::string user_id;
    std::vector<Point2D> screen_points;
    std::vector<PupilInfo> pupil_observations;
};

std::ostream& operator<<(std::ostream& os, const CalibrationProfile& cp);

}  // namespace eyetrack
