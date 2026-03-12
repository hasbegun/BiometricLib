#include <iris/nodes/cross_object_validators.hpp>

#include <string>

#include <iris/core/node_registry.hpp>

namespace iris {

// ===========================================================================
// EyeCentersInsideImageValidator
// ===========================================================================

EyeCentersInsideImageValidator::EyeCentersInsideImageValidator(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("min_distance_to_border");
    if (it != node_params.end())
        params_.min_distance_to_border = std::stod(it->second);
}

Result<std::monostate> EyeCentersInsideImageValidator::run(
    const IRImage& ir_image, const EyeCenters& eye_centers) const {
    if (ir_image.data.empty()) {
        return make_error(ErrorCode::ValidationFailed, "Empty image");
    }

    const auto w = static_cast<double>(ir_image.data.cols);
    const auto h = static_cast<double>(ir_image.data.rows);
    const double d = params_.min_distance_to_border;

    auto check = [&](double cx, double cy,
                     const char* label) -> Result<std::monostate> {
        if (cx < d || cx > w - d || cy < d || cy > h - d) {
            return make_error(
                ErrorCode::ValidationFailed,
                std::string(label)
                    + " center too close to border: ("
                    + std::to_string(cx) + ", " + std::to_string(cy)
                    + ")");
        }
        return std::monostate{};
    };

    auto r = check(eye_centers.pupil_center.x,
                    eye_centers.pupil_center.y, "Pupil");
    if (!r) return r;

    return check(eye_centers.iris_center.x, eye_centers.iris_center.y,
                 "Iris");
}

IRIS_REGISTER_NODE("iris.EyeCentersInsideImageValidator",
                    EyeCentersInsideImageValidator);

// ===========================================================================
// ExtrapolatedPolygonsInsideImageValidator
// ===========================================================================

ExtrapolatedPolygonsInsideImageValidator::
    ExtrapolatedPolygonsInsideImageValidator(
        const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("min_pupil_allowed_percentage");
    if (it != node_params.end())
        params_.min_pupil_allowed_percentage = std::stod(it->second);

    it = node_params.find("min_iris_allowed_percentage");
    if (it != node_params.end())
        params_.min_iris_allowed_percentage = std::stod(it->second);

    it = node_params.find("min_eyeball_allowed_percentage");
    if (it != node_params.end())
        params_.min_eyeball_allowed_percentage = std::stod(it->second);
}

namespace {

double fraction_inside(const Contour& contour, int img_w, int img_h) {
    if (contour.points.empty()) return 1.0;

    int inside = 0;
    for (const auto& pt : contour.points) {
        if (pt.x >= 0.0f
            && pt.x < static_cast<float>(img_w)
            && pt.y >= 0.0f
            && pt.y < static_cast<float>(img_h)) {
            ++inside;
        }
    }
    return static_cast<double>(inside)
           / static_cast<double>(contour.points.size());
}

}  // namespace

Result<std::monostate> ExtrapolatedPolygonsInsideImageValidator::run(
    const IRImage& ir_image,
    const GeometryPolygons& geometry_polygons) const {
    if (ir_image.data.empty()) {
        return make_error(ErrorCode::ValidationFailed, "Empty image");
    }

    const int w = ir_image.data.cols;
    const int h = ir_image.data.rows;

    const double pupil_frac =
        fraction_inside(geometry_polygons.pupil, w, h);
    if (pupil_frac < params_.min_pupil_allowed_percentage) {
        return make_error(ErrorCode::ValidationFailed,
                          "Pupil polygon outside image: "
                              + std::to_string(pupil_frac * 100.0) + "%");
    }

    const double iris_frac =
        fraction_inside(geometry_polygons.iris, w, h);
    if (iris_frac < params_.min_iris_allowed_percentage) {
        return make_error(ErrorCode::ValidationFailed,
                          "Iris polygon outside image: "
                              + std::to_string(iris_frac * 100.0) + "%");
    }

    const double eyeball_frac =
        fraction_inside(geometry_polygons.eyeball, w, h);
    if (eyeball_frac < params_.min_eyeball_allowed_percentage) {
        return make_error(ErrorCode::ValidationFailed,
                          "Eyeball polygon outside image: "
                              + std::to_string(eyeball_frac * 100.0) + "%");
    }

    return std::monostate{};
}

IRIS_REGISTER_NODE("iris.ExtrapolatedPolygonsInsideImageValidator",
                    ExtrapolatedPolygonsInsideImageValidator);

// Long-form aliases matching Python pipeline.yaml class paths
IRIS_REGISTER_NODE_ALIAS(
    "iris.nodes.validators.cross_object_validators.EyeCentersInsideImageValidator",
    EyeCentersInsideImageValidator, EyeCentersInsideImageValidator_long);
IRIS_REGISTER_NODE_ALIAS(
    "iris.nodes.validators.cross_object_validators.ExtrapolatedPolygonsInsideImageValidator",
    ExtrapolatedPolygonsInsideImageValidator, ExtrapolatedPolygonsInsideImageValidator_long);

}  // namespace iris
