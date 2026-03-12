#include <iris/nodes/pupil_iris_property_calculator.hpp>

#include <cmath>
#include <string>

#include <iris/core/node_registry.hpp>
#include <iris/utils/geometry_utils.hpp>

namespace iris {

PupilIrisPropertyCalculator::PupilIrisPropertyCalculator(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("min_pupil_diameter");
    if (it != node_params.end()) params_.min_pupil_diameter = std::stod(it->second);

    it = node_params.find("min_iris_diameter");
    if (it != node_params.end()) params_.min_iris_diameter = std::stod(it->second);
}

Result<PupilToIrisProperty> PupilIrisPropertyCalculator::run(
    const GeometryPolygons& geometries,
    const EyeCenters& eye_centers) const {
    const double iris_diameter = estimate_diameter(geometries.iris);
    const double pupil_diameter = estimate_diameter(geometries.pupil);

    if (pupil_diameter < params_.min_pupil_diameter) {
        return make_error(ErrorCode::GeometryEstimationFailed,
                          "Pupil diameter is too small");
    }
    if (iris_diameter < params_.min_iris_diameter) {
        return make_error(ErrorCode::GeometryEstimationFailed,
                          "Iris diameter is too small");
    }
    if (pupil_diameter >= iris_diameter) {
        return make_error(ErrorCode::GeometryEstimationFailed,
                          "Pupil diameter >= iris diameter");
    }

    const double dx = eye_centers.pupil_center.x - eye_centers.iris_center.x;
    const double dy = eye_centers.pupil_center.y - eye_centers.iris_center.y;
    const double center_dist = std::hypot(dx, dy);

    if (center_dist * 2.0 >= iris_diameter) {
        return make_error(ErrorCode::GeometryEstimationFailed,
                          "Pupil center is outside iris");
    }

    return PupilToIrisProperty{
        .diameter_ratio = pupil_diameter / iris_diameter,
        .center_distance_ratio = center_dist * 2.0 / iris_diameter,
    };
}

IRIS_REGISTER_NODE("iris.PupilIrisPropertyCalculator",
                    PupilIrisPropertyCalculator);

}  // namespace iris
