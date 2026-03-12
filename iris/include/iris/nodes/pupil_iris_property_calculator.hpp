#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Computes pupil-to-iris diameter ratio and center distance ratio.
class PupilIrisPropertyCalculator
    : public Algorithm<PupilIrisPropertyCalculator> {
public:
    static constexpr std::string_view kName = "PupilIrisPropertyCalculator";

    struct Params {
        double min_pupil_diameter = 1.0;
        double min_iris_diameter = 150.0;
    };

    PupilIrisPropertyCalculator() = default;
    explicit PupilIrisPropertyCalculator(const Params& params)
        : params_(params) {}
    explicit PupilIrisPropertyCalculator(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<PupilToIrisProperty> run(const GeometryPolygons& geometries,
                                    const EyeCenters& eye_centers) const;

private:
    Params params_;
};

}  // namespace iris
