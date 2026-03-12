#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Method for selecting the reference template during alignment.
enum class ReferenceSelectionMethod {
    Linear,          // minimise sum of distances
    MeanSquared,     // minimise mean of squared distances
    RootMeanSquared  // minimise RMS of distances
};

/// Align multiple templates by rotating them to a common reference.
///
/// Algorithm:
///   1. Compute pairwise distances and best rotations (via BatchMatcher)
///   2. Select reference template (minimum aggregated distance)
///   3. Rotate all non-reference templates to align with the reference
class HammingDistanceBasedAlignment : public Algorithm<HammingDistanceBasedAlignment> {
public:
    static constexpr std::string_view kName = "HammingDistanceBasedAlignment";

    struct Params {
        int rotation_shift = 15;
        bool normalise = true;
        double norm_mean = 0.45;
        double norm_gradient = 0.00005;
        ReferenceSelectionMethod reference_selection_method = ReferenceSelectionMethod::Linear;
    };

    HammingDistanceBasedAlignment() = default;
    explicit HammingDistanceBasedAlignment(const Params& params) : params_(params) {}
    explicit HammingDistanceBasedAlignment(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<AlignedTemplates> run(const std::vector<IrisTemplateWithId>& templates) const;

    const Params& params() const { return params_; }

private:
    Params params_;

    /// Find the reference template index.
    size_t find_reference(const DistanceMatrix& dm) const;
};

}  // namespace iris
