#include <iris/nodes/contour_extractor.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <opencv2/imgproc.hpp>

#include <iris/core/node_registry.hpp>
#include <iris/utils/geometry_utils.hpp>

namespace iris {

ContourExtractor::ContourExtractor(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto get = [&](const std::string& key, double& out) {
        auto it = node_params.find(key);
        if (it != node_params.end()) {
            out = std::stod(it->second);
        }
    };
    get("relative_area_threshold", params_.relative_area_threshold);
    get("absolute_area_threshold", params_.absolute_area_threshold);
}

namespace {

/// Find the largest parent contour from a binary mask.
/// Returns empty contour if no valid contour found after area filtering.
Contour extract_largest_contour(const cv::Mat& binary_mask,
                                double relative_area_threshold,
                                double absolute_area_threshold) {
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    cv::findContours(binary_mask.clone(), contours, hierarchy,
                     cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        return {};
    }

    // Filter to parent contours only (hierarchy[i][3] == -1 means no parent)
    std::vector<size_t> parent_indices;
    for (size_t i = 0; i < contours.size(); ++i) {
        if (hierarchy[i][3] == -1) {
            parent_indices.push_back(i);
        }
    }

    if (parent_indices.empty()) {
        return {};
    }

    // Compute areas for parent contours
    std::vector<double> areas;
    areas.reserve(parent_indices.size());
    double max_area = 0.0;
    for (auto idx : parent_indices) {
        double a = cv::contourArea(contours[idx]);
        areas.push_back(a);
        max_area = std::max(max_area, a);
    }

    // Filter by area thresholds
    double area_threshold = std::max(absolute_area_threshold,
                                     relative_area_threshold * max_area);
    size_t best_idx = 0;
    double best_area = -1.0;
    for (size_t i = 0; i < parent_indices.size(); ++i) {
        if (areas[i] >= area_threshold && areas[i] > best_area) {
            best_area = areas[i];
            best_idx = parent_indices[i];
        }
    }

    if (best_area < 0.0) {
        return {};
    }

    // Convert cv::Point (int) to cv::Point2f
    Contour result;
    result.points.reserve(contours[best_idx].size());
    for (const auto& pt : contours[best_idx]) {
        result.points.emplace_back(static_cast<float>(pt.x),
                                   static_cast<float>(pt.y));
    }

    return result;
}

}  // namespace

Result<GeometryPolygons> ContourExtractor::run(const GeometryMask& geometry_mask) const {
    // Validate iris mask is non-empty
    if (geometry_mask.iris.empty() || cv::countNonZero(geometry_mask.iris) == 0) {
        return make_error(ErrorCode::VectorizationFailed,
                          "Iris mask is empty — cannot extract contours");
    }

    // Create filled masks for contour extraction.
    // Eyeball mask = pupil | iris | eyeball
    // Iris mask = pupil | iris
    cv::Mat filled_eb = filled_eyeball_mask(geometry_mask);
    cv::Mat filled_ir = filled_iris_mask(geometry_mask);

    GeometryPolygons polygons;

    polygons.eyeball = extract_largest_contour(
        filled_eb, params_.relative_area_threshold, params_.absolute_area_threshold);
    polygons.iris = extract_largest_contour(
        filled_ir, params_.relative_area_threshold, params_.absolute_area_threshold);
    polygons.pupil = extract_largest_contour(
        geometry_mask.pupil, params_.relative_area_threshold, params_.absolute_area_threshold);

    // Verify we found iris contour at minimum
    if (polygons.iris.points.empty()) {
        return make_error(ErrorCode::VectorizationFailed,
                          "No iris contour found after area filtering");
    }

    return polygons;
}

IRIS_REGISTER_NODE("iris.ContouringAlgorithm", ContourExtractor);

}  // namespace iris
