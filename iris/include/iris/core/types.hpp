#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include <iris/core/error.hpp>
#include <iris/core/iris_code_packed.hpp>

namespace iris {

// ---- Enums ----

enum class EyeSide : uint8_t { Left, Right };

// ---- Pipeline data types ----

struct IRImage {
    cv::Mat data;  // CV_8UC1 grayscale
    std::string image_id;
    EyeSide eye_side = EyeSide::Left;
};

struct SegmentationMap {
    cv::Mat predictions;  // CV_32FC(num_classes), e.g. CV_32FC4
    // Class indices: 0=eyeball, 1=iris, 2=pupil, 3=eyelashes
};

struct GeometryMask {
    cv::Mat pupil;    // CV_8UC1 binary
    cv::Mat iris;     // CV_8UC1 binary
    cv::Mat eyeball;  // CV_8UC1 binary
};

struct NoiseMask {
    cv::Mat mask;  // CV_8UC1 binary
};

struct Contour {
    std::vector<cv::Point2f> points;
};

struct GeometryPolygons {
    Contour pupil;
    Contour iris;
    Contour eyeball;
};

struct EyeCenters {
    cv::Point2d pupil_center;
    cv::Point2d iris_center;
};

struct EyeOrientation {
    double angle = 0.0;  // radians
};

struct NormalizedIris {
    cv::Mat normalized_image;  // CV_64FC1
    cv::Mat normalized_mask;   // CV_8UC1 binary
};

struct IrisFilterResponse {
    struct Response {
        cv::Mat data;  // CV_64FC2 (real + imaginary)
        cv::Mat mask;  // CV_64FC2 (real + imaginary mask response)
    };
    std::vector<Response> responses;
    std::string iris_code_version{"v2.0"};
};

struct IrisCode {
    cv::Mat code;  // CV_8UC1 — one byte per bit, shape (16, 512) or (rows, cols*2)
    cv::Mat mask;  // CV_8UC1 same shape
};

// ---- Template types ----

struct IrisTemplate {
    std::vector<PackedIrisCode> iris_codes;
    std::vector<PackedIrisCode> mask_codes;
    std::string iris_code_version{"v2.0"};
};

struct IrisTemplateWithId : IrisTemplate {
    std::string image_id;
};

struct WeightedIrisTemplate : IrisTemplate {
    std::vector<cv::Mat> weights;  // per-bit reliability weights
};

struct DistanceMatrix {
    cv::Mat data;  // CV_64FC1, symmetric
    size_t size = 0;

    explicit DistanceMatrix(size_t n = 0)
        : data(cv::Mat::zeros(static_cast<int>(n), static_cast<int>(n), CV_64FC1)), size(n) {}

    double& operator()(size_t i, size_t j) {
        return data.at<double>(static_cast<int>(i), static_cast<int>(j));
    }

    double operator()(size_t i, size_t j) const {
        return data.at<double>(static_cast<int>(i), static_cast<int>(j));
    }
};

struct AlignedTemplates {
    std::vector<IrisTemplateWithId> templates;
    DistanceMatrix distances;
    std::string reference_template_id;
};

// ---- Quality metrics ----

struct Landmarks {
    std::vector<cv::Point2f> pupil_landmarks;
    std::vector<cv::Point2f> iris_landmarks;
    std::vector<cv::Point2f> eyeball_landmarks;
};

struct Offgaze {
    double score = 0.0;
};

struct Sharpness {
    double score = 0.0;
};

struct EyeOcclusion {
    double fraction = 0.0;
};

struct PupilToIrisProperty {
    double diameter_ratio = 0.0;
    double center_distance_ratio = 0.0;
};

struct BoundingBox {
    double x_min = 0.0;
    double y_min = 0.0;
    double x_max = 0.0;
    double y_max = 0.0;
};

// ---- Pipeline output ----

struct PipelineOutput {
    std::optional<IrisTemplate> iris_template;
    std::optional<IrisError> error;
};

}  // namespace iris
