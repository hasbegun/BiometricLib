#include <iris/nodes/template_alignment.hpp>

#include <cmath>
#include <limits>
#include <numeric>

#include <iris/core/node_registry.hpp>
#include <iris/nodes/batch_matcher.hpp>

namespace iris {

// --- Construction ---

HammingDistanceBasedAlignment::HammingDistanceBasedAlignment(
    const std::unordered_map<std::string, std::string>& node_params) {
    if (auto it = node_params.find("rotation_shift"); it != node_params.end())
        params_.rotation_shift = std::stoi(it->second);
    if (auto it = node_params.find("normalise"); it != node_params.end())
        params_.normalise = (it->second == "true" || it->second == "1");
    if (auto it = node_params.find("norm_mean"); it != node_params.end())
        params_.norm_mean = std::stod(it->second);
    if (auto it = node_params.find("norm_gradient"); it != node_params.end())
        params_.norm_gradient = std::stod(it->second);
    if (auto it = node_params.find("reference_selection_method"); it != node_params.end()) {
        if (it->second == "linear")
            params_.reference_selection_method = ReferenceSelectionMethod::Linear;
        else if (it->second == "mean_squared")
            params_.reference_selection_method = ReferenceSelectionMethod::MeanSquared;
        else if (it->second == "root_mean_squared")
            params_.reference_selection_method = ReferenceSelectionMethod::RootMeanSquared;
    }
}

// --- Reference selection ---

size_t HammingDistanceBasedAlignment::find_reference(const DistanceMatrix& dm) const {
    const size_t n = dm.size;
    if (n <= 1) return 0;

    size_t best_idx = 0;
    double best_score = std::numeric_limits<double>::max();

    for (size_t i = 0; i < n; ++i) {
        double score = 0.0;
        for (size_t j = 0; j < n; ++j) {
            if (i == j) continue;
            switch (params_.reference_selection_method) {
                case ReferenceSelectionMethod::Linear:
                    score += dm(i, j);
                    break;
                case ReferenceSelectionMethod::MeanSquared:
                    score += dm(i, j) * dm(i, j);
                    break;
                case ReferenceSelectionMethod::RootMeanSquared:
                    score += dm(i, j) * dm(i, j);
                    break;
            }
        }

        // Normalise by number of comparisons
        const double denom = static_cast<double>(n - 1);
        switch (params_.reference_selection_method) {
            case ReferenceSelectionMethod::Linear:
                score /= denom;
                break;
            case ReferenceSelectionMethod::MeanSquared:
                score /= denom;
                break;
            case ReferenceSelectionMethod::RootMeanSquared:
                score = std::sqrt(score / denom);
                break;
        }

        if (score < best_score) {
            best_score = score;
            best_idx = i;
        }
    }

    return best_idx;
}

// --- Main algorithm ---

Result<AlignedTemplates> HammingDistanceBasedAlignment::run(
    const std::vector<IrisTemplateWithId>& templates) const {

    if (templates.empty()) {
        return make_error(ErrorCode::MatchingFailed, "No templates to align");
    }

    // Single template: nothing to align
    if (templates.size() == 1) {
        AlignedTemplates at;
        at.templates = templates;
        at.distances = DistanceMatrix(1);
        at.reference_template_id = templates[0].image_id;
        return at;
    }

    // Step 1: Compute pairwise distances and rotations
    BatchMatcher::Params bm_params;
    bm_params.rotation_shift = params_.rotation_shift;
    bm_params.normalise = params_.normalise;
    bm_params.norm_mean = params_.norm_mean;
    bm_params.norm_gradient = params_.norm_gradient;
    BatchMatcher bm(bm_params);

    // Convert to base IrisTemplate for BatchMatcher
    std::vector<IrisTemplate> base_templates(templates.begin(), templates.end());
    auto pw_result = bm.match_n_vs_n_with_rotations(base_templates);
    if (!pw_result.has_value()) {
        return std::unexpected(pw_result.error());
    }

    auto& pw = pw_result.value();

    // Step 2: Find reference template
    const size_t ref_idx = find_reference(pw.distances);

    // Step 3: Rotate all templates to align with reference
    AlignedTemplates at;
    at.distances = std::move(pw.distances);
    at.reference_template_id = templates[ref_idx].image_id;
    at.templates.reserve(templates.size());

    for (size_t i = 0; i < templates.size(); ++i) {
        if (i == ref_idx) {
            at.templates.push_back(templates[i]);
            continue;
        }

        // Get the best rotation from template i to reference
        int rotation = pw.rotations.at<int32_t>(
            static_cast<int>(i), static_cast<int>(ref_idx));

        IrisTemplateWithId aligned = templates[i];
        for (size_t w = 0; w < aligned.iris_codes.size(); ++w) {
            aligned.iris_codes[w] = aligned.iris_codes[w].rotate(rotation);
            aligned.mask_codes[w] = aligned.mask_codes[w].rotate(rotation);
        }
        at.templates.push_back(std::move(aligned));
    }

    return at;
}

IRIS_REGISTER_NODE("iris.HammingDistanceBasedAlignment", HammingDistanceBasedAlignment);

}  // namespace iris
