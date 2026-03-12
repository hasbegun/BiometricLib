#include <iris/nodes/object_validators.hpp>

#include <bit>
#include <cmath>
#include <string>

#include <opencv2/imgproc.hpp>

#include <iris/core/node_registry.hpp>
#include <iris/utils/geometry_utils.hpp>

namespace iris {

// ===========================================================================
// Pupil2IrisPropertyValidator
// ===========================================================================

Pupil2IrisPropertyValidator::Pupil2IrisPropertyValidator(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("min_allowed_diameter_ratio");
    if (it != node_params.end())
        params_.min_allowed_diameter_ratio = std::stod(it->second);

    it = node_params.find("max_allowed_diameter_ratio");
    if (it != node_params.end())
        params_.max_allowed_diameter_ratio = std::stod(it->second);

    it = node_params.find("max_allowed_center_dist_ratio");
    if (it != node_params.end())
        params_.max_allowed_center_dist_ratio = std::stod(it->second);
}

Result<std::monostate> Pupil2IrisPropertyValidator::run(
    const PupilToIrisProperty& property) const {
    if (property.diameter_ratio < params_.min_allowed_diameter_ratio) {
        return make_error(ErrorCode::ValidationFailed,
                          "Pupil too constricted: diameter_ratio="
                              + std::to_string(property.diameter_ratio));
    }
    if (property.diameter_ratio > params_.max_allowed_diameter_ratio) {
        return make_error(ErrorCode::ValidationFailed,
                          "Pupil too dilated: diameter_ratio="
                              + std::to_string(property.diameter_ratio));
    }
    if (property.center_distance_ratio
        > params_.max_allowed_center_dist_ratio) {
        return make_error(ErrorCode::ValidationFailed,
                          "Pupil too off-center: center_distance_ratio="
                              + std::to_string(property.center_distance_ratio));
    }
    return std::monostate{};
}

IRIS_REGISTER_NODE("iris.Pupil2IrisPropertyValidator",
                    Pupil2IrisPropertyValidator);

// ===========================================================================
// OffgazeValidator
// ===========================================================================

OffgazeValidator::OffgazeValidator(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("max_allowed_offgaze");
    if (it != node_params.end())
        params_.max_allowed_offgaze = std::stod(it->second);
}

Result<std::monostate> OffgazeValidator::run(
    const Offgaze& offgaze) const {
    if (offgaze.score > params_.max_allowed_offgaze) {
        return make_error(ErrorCode::ValidationFailed,
                          "Offgaze too high: score="
                              + std::to_string(offgaze.score));
    }
    return std::monostate{};
}

IRIS_REGISTER_NODE("iris.OffgazeValidator", OffgazeValidator);

// ===========================================================================
// OcclusionValidator
// ===========================================================================

OcclusionValidator::OcclusionValidator(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("min_allowed_occlusion");
    if (it != node_params.end())
        params_.min_allowed_occlusion = std::stod(it->second);
}

Result<std::monostate> OcclusionValidator::run(
    const EyeOcclusion& occlusion) const {
    if (occlusion.fraction < params_.min_allowed_occlusion) {
        return make_error(ErrorCode::ValidationFailed,
                          "Occlusion too high: visible_fraction="
                              + std::to_string(occlusion.fraction));
    }
    return std::monostate{};
}

IRIS_REGISTER_NODE("iris.OcclusionValidator", OcclusionValidator);

// ===========================================================================
// IsPupilInsideIrisValidator
// ===========================================================================

IsPupilInsideIrisValidator::IsPupilInsideIrisValidator(
    const std::unordered_map<std::string, std::string>& /*node_params*/) {}

Result<std::monostate> IsPupilInsideIrisValidator::run(
    const GeometryPolygons& geometry_polygons) const {
    const auto& pupil_pts = geometry_polygons.pupil.points;
    const auto& iris_pts = geometry_polygons.iris.points;

    if (iris_pts.empty() || pupil_pts.empty()) {
        return make_error(ErrorCode::ValidationFailed,
                          "Empty pupil or iris polygon");
    }

    // Convert iris polygon to cv::Point2f for pointPolygonTest
    std::vector<cv::Point2f> iris_contour;
    iris_contour.reserve(iris_pts.size());
    for (const auto& pt : iris_pts) {
        iris_contour.emplace_back(pt.x, pt.y);
    }

    for (const auto& pt : pupil_pts) {
        const double dist = cv::pointPolygonTest(
            iris_contour, cv::Point2f(pt.x, pt.y), false);
        if (dist < 0.0) {
            return make_error(ErrorCode::ValidationFailed,
                              "Pupil point outside iris polygon");
        }
    }

    return std::monostate{};
}

IRIS_REGISTER_NODE("iris.IsPupilInsideIrisValidator",
                    IsPupilInsideIrisValidator);

// ===========================================================================
// SharpnessValidator
// ===========================================================================

SharpnessValidator::SharpnessValidator(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("min_sharpness");
    if (it != node_params.end())
        params_.min_sharpness = std::stod(it->second);
}

Result<std::monostate> SharpnessValidator::run(
    const Sharpness& sharpness) const {
    if (sharpness.score < params_.min_sharpness) {
        return make_error(ErrorCode::ValidationFailed,
                          "Image too blurry: sharpness="
                              + std::to_string(sharpness.score));
    }
    return std::monostate{};
}

IRIS_REGISTER_NODE("iris.SharpnessValidator", SharpnessValidator);

// ===========================================================================
// IsMaskTooSmallValidator
// ===========================================================================

IsMaskTooSmallValidator::IsMaskTooSmallValidator(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("min_maskcodes_size");
    if (it != node_params.end())
        params_.min_maskcodes_size = std::stoi(it->second);
}

Result<std::monostate> IsMaskTooSmallValidator::run(
    const IrisTemplate& iris_template) const {
    // Sum the popcount of all mask codes
    int64_t total_mask_bits = 0;
    for (const auto& mc : iris_template.mask_codes) {
        for (const auto word : mc.mask_bits) {
            total_mask_bits += std::popcount(word);
        }
    }

    if (total_mask_bits < static_cast<int64_t>(params_.min_maskcodes_size)) {
        return make_error(ErrorCode::ValidationFailed,
                          "Mask too small: mask_bits="
                              + std::to_string(total_mask_bits));
    }
    return std::monostate{};
}

IRIS_REGISTER_NODE("iris.IsMaskTooSmallValidator", IsMaskTooSmallValidator);

// ===========================================================================
// PolygonsLengthValidator
// ===========================================================================

PolygonsLengthValidator::PolygonsLengthValidator(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("min_iris_length");
    if (it != node_params.end())
        params_.min_iris_length = std::stod(it->second);

    it = node_params.find("min_pupil_length");
    if (it != node_params.end())
        params_.min_pupil_length = std::stod(it->second);
}

Result<std::monostate> PolygonsLengthValidator::run(
    const GeometryPolygons& geometry_polygons) const {
    const double iris_len = polygon_length(geometry_polygons.iris);
    const double pupil_len = polygon_length(geometry_polygons.pupil);

    if (iris_len < params_.min_iris_length) {
        return make_error(ErrorCode::ValidationFailed,
                          "Iris polygon too short: length="
                              + std::to_string(iris_len));
    }
    if (pupil_len < params_.min_pupil_length) {
        return make_error(ErrorCode::ValidationFailed,
                          "Pupil polygon too short: length="
                              + std::to_string(pupil_len));
    }
    return std::monostate{};
}

IRIS_REGISTER_NODE("iris.PolygonsLengthValidator", PolygonsLengthValidator);

// Long-form aliases matching Python pipeline.yaml class paths
IRIS_REGISTER_NODE_ALIAS(
    "iris.nodes.validators.object_validators.Pupil2IrisPropertyValidator",
    Pupil2IrisPropertyValidator, Pupil2IrisPropertyValidator_long);
IRIS_REGISTER_NODE_ALIAS(
    "iris.nodes.validators.object_validators.OffgazeValidator",
    OffgazeValidator, OffgazeValidator_long);
IRIS_REGISTER_NODE_ALIAS(
    "iris.nodes.validators.object_validators.OcclusionValidator",
    OcclusionValidator, OcclusionValidator_long);
IRIS_REGISTER_NODE_ALIAS(
    "iris.nodes.validators.object_validators.IsPupilInsideIrisValidator",
    IsPupilInsideIrisValidator, IsPupilInsideIrisValidator_long);
IRIS_REGISTER_NODE_ALIAS(
    "iris.nodes.validators.object_validators.SharpnessValidator",
    SharpnessValidator, SharpnessValidator_long);
IRIS_REGISTER_NODE_ALIAS(
    "iris.nodes.validators.object_validators.IsMaskTooSmallValidator",
    IsMaskTooSmallValidator, IsMaskTooSmallValidator_long);
IRIS_REGISTER_NODE_ALIAS(
    "iris.nodes.validators.object_validators.PolygonsLengthValidator",
    PolygonsLengthValidator, PolygonsLengthValidator_long);

}  // namespace iris
