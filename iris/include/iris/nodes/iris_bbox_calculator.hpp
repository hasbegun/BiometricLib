#pragma once

#include <string_view>
#include <unordered_map>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Compute the smallest bounding box around the iris polygon.
class IrisBBoxCalculator : public Algorithm<IrisBBoxCalculator> {
public:
    static constexpr std::string_view kName = "IrisBBoxCalculator";

    struct Params {
        /// Multiplicative buffer: 1.0 = no padding, 2.0 = double size.
        double buffer = 0.0;
    };

    IrisBBoxCalculator() = default;
    explicit IrisBBoxCalculator(const Params& params) : params_(params) {}
    explicit IrisBBoxCalculator(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<BoundingBox> run(const IRImage& ir_image,
                            const GeometryPolygons& geometry_polygons) const;

private:
    Params params_;
};

}  // namespace iris
