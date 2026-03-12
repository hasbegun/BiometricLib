#pragma once

#include <string_view>
#include <unordered_map>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Estimates eye centers (pupil and iris) using perpendicular bisectors.
///
/// Randomly samples point pairs from the polygon boundary, computes
/// perpendicular bisectors, and finds their least-squares intersection.
class BisectorsEyeCenter : public Algorithm<BisectorsEyeCenter> {
public:
    static constexpr std::string_view kName = "BisectorsEyeCenter";

    struct Params {
        int num_bisectors = 100;
        double min_distance_between_sector_points = 0.75;
        int max_iterations = 50;
    };

    BisectorsEyeCenter() = default;
    explicit BisectorsEyeCenter(const Params& params) : params_(params) {}
    explicit BisectorsEyeCenter(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<EyeCenters> run(const GeometryPolygons& polygons) const;

private:
    Params params_;
};

}  // namespace iris
