#include <iris/nodes/template_identity_filter.hpp>

#include <algorithm>
#include <queue>
#include <vector>

#include <iris/core/node_registry.hpp>

namespace iris {

TemplateIdentityFilter::TemplateIdentityFilter(
    const std::unordered_map<std::string, std::string>& node_params) {
    if (auto it = node_params.find("identity_distance_threshold"); it != node_params.end())
        params_.identity_distance_threshold = std::stod(it->second);
    if (auto it = node_params.find("validation_action"); it != node_params.end()) {
        if (it->second == "remove")
            params_.validation_action = IdentityValidationAction::Remove;
        else if (it->second == "raise_error")
            params_.validation_action = IdentityValidationAction::RaiseError;
    }
    if (auto it = node_params.find("min_templates_after_validation"); it != node_params.end())
        params_.min_templates_after_validation = static_cast<size_t>(std::stoul(it->second));
}

Result<AlignedTemplates> TemplateIdentityFilter::run(const AlignedTemplates& input) const {
    if (input.templates.empty()) {
        return make_error(ErrorCode::IdentityValidationFailed, "No templates to filter");
    }

    const size_t n = input.templates.size();

    if (n == 1) {
        return input;
    }

    // Build adjacency list: edge if distance <= threshold
    std::vector<std::vector<size_t>> adj(n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            if (input.distances(i, j) <= params_.identity_distance_threshold) {
                adj[i].push_back(j);
                adj[j].push_back(i);
            }
        }
    }

    // BFS to find connected components
    std::vector<int> component(n, -1);
    std::vector<std::vector<size_t>> components;
    int comp_id = 0;

    for (size_t i = 0; i < n; ++i) {
        if (component[i] >= 0) continue;

        std::vector<size_t> comp;
        std::queue<size_t> q;
        q.push(i);
        component[i] = comp_id;

        while (!q.empty()) {
            size_t cur = q.front();
            q.pop();
            comp.push_back(cur);

            for (size_t nb : adj[cur]) {
                if (component[nb] < 0) {
                    component[nb] = comp_id;
                    q.push(nb);
                }
            }
        }

        components.push_back(std::move(comp));
        ++comp_id;
    }

    // Find largest component
    size_t largest_idx = 0;
    for (size_t c = 1; c < components.size(); ++c) {
        if (components[c].size() > components[largest_idx].size()) {
            largest_idx = c;
        }
    }

    const auto& largest = components[largest_idx];
    const bool has_outliers = largest.size() < n;

    // Handle outliers
    if (has_outliers &&
        params_.validation_action == IdentityValidationAction::RaiseError) {
        return make_error(ErrorCode::IdentityValidationFailed,
            "Outlier templates detected in identity group");
    }

    // Check minimum templates requirement
    if (largest.size() < params_.min_templates_after_validation) {
        return make_error(ErrorCode::IdentityValidationFailed,
            "Too few templates remain after identity filtering ("
            + std::to_string(largest.size()) + " < "
            + std::to_string(params_.min_templates_after_validation) + ")");
    }

    // Build filtered result
    AlignedTemplates result;
    result.reference_template_id = input.reference_template_id;

    // Sort indices for stable output order
    auto sorted_indices = largest;
    std::sort(sorted_indices.begin(), sorted_indices.end());

    // Build new distance matrix
    const size_t m = sorted_indices.size();
    result.distances = DistanceMatrix(m);
    for (size_t ri = 0; ri < m; ++ri) {
        for (size_t rj = ri + 1; rj < m; ++rj) {
            double d = input.distances(sorted_indices[ri], sorted_indices[rj]);
            result.distances(ri, rj) = d;
            result.distances(rj, ri) = d;
        }
    }

    // Copy templates
    for (size_t idx : sorted_indices) {
        result.templates.push_back(input.templates[idx]);
    }

    return result;
}

IRIS_REGISTER_NODE("iris.TemplateIdentityFilter", TemplateIdentityFilter);

}  // namespace iris
