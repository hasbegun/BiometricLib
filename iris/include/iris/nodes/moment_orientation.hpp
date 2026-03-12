#pragma once

#include <string_view>
#include <unordered_map>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Estimates eye orientation using second-order image moments of the eyeball.
///
/// Computes the principal axis angle of the eyeball polygon from central
/// moments (mu20, mu02, mu11). Rejects near-circular shapes whose
/// eccentricity falls below a threshold.
class MomentOrientation : public Algorithm<MomentOrientation> {
public:
    static constexpr std::string_view kName = "MomentOrientation";

    struct Params {
        double eccentricity_threshold = 0.1;
    };

    MomentOrientation() = default;
    explicit MomentOrientation(const Params& params) : params_(params) {}
    explicit MomentOrientation(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<EyeOrientation> run(const GeometryPolygons& polygons) const;

private:
    Params params_;
};

}  // namespace iris
