#include <eyetrack/nodes/pupil_detector.hpp>

#include <opencv2/imgproc.hpp>

#include <eyetrack/core/node_registry.hpp>

namespace eyetrack {

const std::vector<std::string> PupilDetector::kDependencies = {"eyetrack.EyeExtractor"};

PupilDetector::PupilDetector() = default;

PupilDetector::PupilDetector(const Config& config) : config_(config) {}

PupilDetector::PupilDetector(const NodeParams& params) {
    if (auto it = params.find("method"); it != params.end()) {
        if (it->second == "ellipse")
            config_.method = PupilDetectorMethod::Ellipse;
        else
            config_.method = PupilDetectorMethod::Centroid;
    }
    if (auto it = params.find("adaptive_block_size"); it != params.end())
        config_.adaptive_block_size = std::stoi(it->second);
    if (auto it = params.find("adaptive_c"); it != params.end())
        config_.adaptive_c = std::stod(it->second);
}

Result<PupilInfo> PupilDetector::run(const cv::Mat& eye_crop) const {
    if (eye_crop.empty()) {
        return make_error(ErrorCode::InvalidInput, "Empty eye crop");
    }

    // Convert to grayscale if needed
    cv::Mat gray;
    if (eye_crop.channels() == 3) {
        cv::cvtColor(eye_crop, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = eye_crop;
    }

    switch (config_.method) {
        case PupilDetectorMethod::Centroid:
            return run_centroid(gray);
        case PupilDetectorMethod::Ellipse:
            return run_ellipse(gray);
    }
    return make_error(ErrorCode::InvalidInput, "Unknown pupil detector method");
}

Result<PupilInfo> PupilDetector::run_centroid(const cv::Mat& gray) const {
    // Adaptive threshold to find dark pupil regions
    cv::Mat thresh;
    cv::adaptiveThreshold(gray, thresh, 255,
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY_INV,
                          config_.adaptive_block_size,
                          config_.adaptive_c);

    // Morphological close to fill gaps
    cv::Mat kernel = cv::getStructuringElement(
        cv::MORPH_ELLIPSE,
        cv::Size(config_.morph_kernel_size, config_.morph_kernel_size));
    cv::morphologyEx(thresh, thresh, cv::MORPH_CLOSE, kernel);

    // Find contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        return make_error(ErrorCode::PupilNotDetected, "No contours found");
    }

    // Find the largest contour by area
    int best_idx = -1;
    double best_area = 0.0;
    for (int i = 0; i < static_cast<int>(contours.size()); ++i) {
        double area = cv::contourArea(contours[static_cast<size_t>(i)]);
        if (area > best_area && area >= config_.min_contour_area) {
            best_area = area;
            best_idx = i;
        }
    }

    if (best_idx < 0) {
        return make_error(ErrorCode::PupilNotDetected,
                          "No contour meets minimum area threshold");
    }

    const auto& best = contours[static_cast<size_t>(best_idx)];

    // Compute centroid using moments
    cv::Moments m = cv::moments(best);
    if (m.m00 < 1e-10) {
        return make_error(ErrorCode::PupilNotDetected, "Degenerate contour moments");
    }

    float cx = static_cast<float>(m.m10 / m.m00);
    float cy = static_cast<float>(m.m01 / m.m00);

    // Position constraint: pupil must be in center 80% of crop
    // Edge detections are likely eyebrow shadow or eyelid
    auto cols_f = static_cast<float>(gray.cols);
    auto rows_f = static_cast<float>(gray.rows);
    if (cx < cols_f * 0.1F || cx > cols_f * 0.9F ||
        cy < rows_f * 0.1F || cy > rows_f * 0.9F) {
        return make_error(ErrorCode::PupilNotDetected, "Centroid at crop edge");
    }

    // Circularity constraint: 4π·area/perimeter² > 0.4
    // Filters out elongated shadows (eyebrows, eyelids)
    double perimeter = cv::arcLength(best, true);
    if (perimeter > 0) {
        double circularity = 4.0 * CV_PI * best_area / (perimeter * perimeter);
        if (circularity < 0.4) {
            return make_error(ErrorCode::PupilNotDetected,
                              "Non-circular contour");
        }
    }

    // Compute radius from minimum enclosing circle
    cv::Point2f center_cv;
    float radius = 0.0F;
    cv::minEnclosingCircle(best, center_cv, radius);

    // Confidence based on contour area vs enclosing circle area
    double circle_area = CV_PI * static_cast<double>(radius) * static_cast<double>(radius);
    float confidence = (circle_area > 1e-10)
                           ? static_cast<float>(best_area / circle_area)
                           : 0.0F;
    confidence = std::min(confidence, 1.0F);

    PupilInfo result;
    result.center = {cx, cy};
    result.radius = radius;
    result.confidence = confidence;
    return result;
}

Result<PupilInfo> PupilDetector::run_ellipse(const cv::Mat& gray) const {
    cv::Mat thresh;
    cv::adaptiveThreshold(gray, thresh, 255,
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY_INV,
                          config_.adaptive_block_size,
                          config_.adaptive_c);

    cv::Mat kernel = cv::getStructuringElement(
        cv::MORPH_ELLIPSE,
        cv::Size(config_.morph_kernel_size, config_.morph_kernel_size));
    cv::morphologyEx(thresh, thresh, cv::MORPH_CLOSE, kernel);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        return make_error(ErrorCode::PupilNotDetected, "No contours found");
    }

    // Find the largest contour
    int best_idx = -1;
    double best_area = 0.0;
    for (int i = 0; i < static_cast<int>(contours.size()); ++i) {
        double area = cv::contourArea(contours[static_cast<size_t>(i)]);
        if (area > best_area && area >= config_.min_contour_area) {
            best_area = area;
            best_idx = i;
        }
    }

    if (best_idx < 0) {
        return make_error(ErrorCode::PupilNotDetected,
                          "No contour meets minimum area threshold");
    }

    const auto& best = contours[static_cast<size_t>(best_idx)];

    // fitEllipse requires at least 5 points
    if (best.size() < 5) {
        return make_error(ErrorCode::PupilNotDetected,
                          "Contour too small for ellipse fitting");
    }

    cv::RotatedRect ellipse = cv::fitEllipse(best);

    // Use average of semi-axes as radius
    float radius = (ellipse.size.width + ellipse.size.height) / 4.0F;

    // Confidence: ratio of contour area to ellipse area
    double ellipse_area =
        CV_PI * (static_cast<double>(ellipse.size.width) / 2.0) * (static_cast<double>(ellipse.size.height) / 2.0);
    float confidence = (ellipse_area > 1e-10)
                           ? static_cast<float>(best_area / ellipse_area)
                           : 0.0F;
    confidence = std::min(confidence, 1.0F);

    PupilInfo result;
    result.center = {ellipse.center.x, ellipse.center.y};
    result.radius = radius;
    result.confidence = confidence;
    return result;
}

Result<void> PupilDetector::execute(
    std::unordered_map<std::string, std::any>& context) const {
    // Detect left pupil
    if (auto it = context.find("left_eye_crop"); it != context.end()) {
        const auto& crop = std::any_cast<const cv::Mat&>(it->second);
        auto result = run(crop);
        if (result.has_value()) {
            context["left_pupil"] = result.value();
        }
    }

    // Detect right pupil
    if (auto it = context.find("right_eye_crop"); it != context.end()) {
        const auto& crop = std::any_cast<const cv::Mat&>(it->second);
        auto result = run(crop);
        if (result.has_value()) {
            context["right_pupil"] = result.value();
        }
    }

    // At least one pupil must be detected
    if (context.find("left_pupil") == context.end() &&
        context.find("right_pupil") == context.end()) {
        return make_error(ErrorCode::PupilNotDetected,
                          "Neither pupil detected");
    }

    return {};
}

const std::vector<std::string>& PupilDetector::dependencies() const noexcept {
    return kDependencies;
}

EYETRACK_REGISTER_NODE("eyetrack.PupilDetector", PupilDetector);

}  // namespace eyetrack
