#include <iris/nodes/iris_bbox_calculator.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

#include <iris/core/node_registry.hpp>

namespace iris {

IrisBBoxCalculator::IrisBBoxCalculator(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("buffer");
    if (it != node_params.end()) params_.buffer = std::stod(it->second);
}

Result<BoundingBox> IrisBBoxCalculator::run(
    const IRImage& ir_image,
    const GeometryPolygons& geometry_polygons) const {
    const auto& pts = geometry_polygons.iris.points;
    if (pts.empty()) {
        return make_error(ErrorCode::GeometryEstimationFailed,
                          "Empty iris contour for bounding box");
    }

    double x_min = std::numeric_limits<double>::max();
    double y_min = std::numeric_limits<double>::max();
    double x_max = std::numeric_limits<double>::lowest();
    double y_max = std::numeric_limits<double>::lowest();

    for (const auto& p : pts) {
        const double px = static_cast<double>(p.x);
        const double py = static_cast<double>(p.y);
        x_min = std::min(x_min, px);
        y_min = std::min(y_min, py);
        x_max = std::max(x_max, px);
        y_max = std::max(y_max, py);
    }

    if (x_max <= x_min || y_max <= y_min) {
        return make_error(ErrorCode::GeometryEstimationFailed,
                          "Degenerate iris bounding box");
    }

    // Apply buffer as multiplicative factor
    if (params_.buffer > 0.0) {
        const double w = x_max - x_min;
        const double h = y_max - y_min;
        const double pad_x = w * (params_.buffer - 1.0) / 2.0;
        const double pad_y = h * (params_.buffer - 1.0) / 2.0;
        x_min -= pad_x;
        x_max += pad_x;
        y_min -= pad_y;
        y_max += pad_y;
    }

    // Clamp to image bounds
    const double img_w = static_cast<double>(ir_image.data.cols);
    const double img_h = static_cast<double>(ir_image.data.rows);
    x_min = std::max(x_min, 0.0);
    y_min = std::max(y_min, 0.0);
    x_max = std::min(x_max, img_w);
    y_max = std::min(y_max, img_h);

    return BoundingBox{
        .x_min = x_min, .y_min = y_min, .x_max = x_max, .y_max = y_max};
}

IRIS_REGISTER_NODE("iris.IrisBBoxCalculator", IrisBBoxCalculator);

}  // namespace iris
