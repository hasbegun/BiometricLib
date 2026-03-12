#include <eyetrack/utils/calibration_utils.hpp>

#include <eyetrack/utils/geometry_utils.hpp>

namespace eyetrack {

Result<Eigen::MatrixXf> least_squares_fit(
    const std::vector<Point2D>& pupil_positions,
    const std::vector<Point2D>& screen_points) {
    if (pupil_positions.size() < 3) {
        return make_error(ErrorCode::CalibrationFailed,
                          "Need at least 3 calibration points, got " +
                              std::to_string(pupil_positions.size()));
    }
    if (pupil_positions.size() != screen_points.size()) {
        return make_error(ErrorCode::CalibrationFailed,
                          "Mismatched point counts");
    }

    auto n = static_cast<Eigen::Index>(pupil_positions.size());

    // Build design matrix A (n x 6) using polynomial features [1, x, y, x², y², xy]
    Eigen::MatrixXf A(n, 6);
    for (Eigen::Index i = 0; i < n; ++i) {
        auto features = polynomial_features(
            pupil_positions[static_cast<size_t>(i)].x,
            pupil_positions[static_cast<size_t>(i)].y);
        for (int j = 0; j < 6; ++j) {
            A(i, j) = features[static_cast<size_t>(j)];
        }
    }

    // Build target matrix B (n x 2)
    Eigen::MatrixXf B(n, 2);
    for (Eigen::Index i = 0; i < n; ++i) {
        B(i, 0) = screen_points[static_cast<size_t>(i)].x;
        B(i, 1) = screen_points[static_cast<size_t>(i)].y;
    }

    // Solve A * coeffs = B using least squares (QR decomposition)
    Eigen::MatrixXf coeffs = A.colPivHouseholderQr().solve(B);

    return coeffs;  // 6 x 2 matrix
}

Point2D apply_polynomial_mapping(
    Point2D pupil,
    const Eigen::MatrixXf& coeffs) {
    auto features = polynomial_features(pupil.x, pupil.y);

    Eigen::VectorXf f(6);
    for (int i = 0; i < 6; ++i) {
        f(i) = features[static_cast<size_t>(i)];
    }

    Eigen::VectorXf result = coeffs.transpose() * f;
    return {result(0), result(1)};
}

}  // namespace eyetrack
