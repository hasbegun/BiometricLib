#pragma once

#include <string_view>
#include <unordered_map>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Action to take when an outlier template is detected.
enum class IdentityValidationAction : uint8_t {
    Remove,     // silently remove outlier templates
    RaiseError  // return an error if any outlier is found
};

/// Filter out templates that don't belong to the same identity.
///
/// Algorithm:
///   1. Build adjacency graph: edge (i,j) if distance(i,j) <= threshold
///   2. BFS to find connected components
///   3. Keep the largest component, handle outliers per validation_action
class TemplateIdentityFilter : public Algorithm<TemplateIdentityFilter> {
public:
    static constexpr std::string_view kName = "TemplateIdentityFilter";

    struct Params {
        double identity_distance_threshold = 0.35;
        IdentityValidationAction validation_action = IdentityValidationAction::Remove;
        size_t min_templates_after_validation = 1;
    };

    TemplateIdentityFilter() = default;
    explicit TemplateIdentityFilter(const Params& params) : params_(params) {}
    explicit TemplateIdentityFilter(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<AlignedTemplates> run(const AlignedTemplates& input) const;

    const Params& params() const { return params_; }

private:
    Params params_;
};

}  // namespace iris
