#pragma once

#include <string_view>
#include <unordered_map>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Compute iris sharpness as the standard deviation of the Laplacian
/// within the unmasked region of the normalized iris.
class SharpnessEstimation : public Algorithm<SharpnessEstimation> {
public:
    static constexpr std::string_view kName = "SharpnessEstimation";

    struct Params {
        int lap_ksize = 11;         // Laplacian kernel size (must be odd)
        int erosion_ksize_w = 29;   // Erosion kernel width (must be odd)
        int erosion_ksize_h = 15;   // Erosion kernel height (must be odd)
    };

    SharpnessEstimation() = default;
    explicit SharpnessEstimation(const Params& params) : params_(params) {}
    explicit SharpnessEstimation(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<Sharpness> run(const NormalizedIris& normalization_output) const;

private:
    Params params_;
};

}  // namespace iris
