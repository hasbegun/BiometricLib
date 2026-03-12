#include <eyetrack/utils/geometry_utils.hpp>

#include <cmath>

namespace eyetrack {

float euclidean_distance(Point2D a, Point2D b) noexcept {
    return std::hypot(a.x - b.x, a.y - b.y);
}

float compute_ear(const std::array<Point2D, 6>& eye) noexcept {
    // Vertical distances
    float v1 = euclidean_distance(eye[1], eye[5]);
    float v2 = euclidean_distance(eye[2], eye[4]);

    // Horizontal distance
    float h = euclidean_distance(eye[0], eye[3]);

    if (h < 1e-10F) return 0.0F;

    return (v1 + v2) / (2.0F * h);
}

float compute_ear_average(const EyeLandmarks& landmarks) noexcept {
    return (compute_ear(landmarks.left) + compute_ear(landmarks.right)) / 2.0F;
}

Point2D normalize_point(Point2D p, float width, float height) noexcept {
    if (width < 1e-10F || height < 1e-10F) return {0.0F, 0.0F};
    return {p.x / width, p.y / height};
}

std::vector<float> polynomial_features(float x, float y) {
    return {1.0F, x, y, x * x, y * y, x * y};
}

}  // namespace eyetrack
