#include <iris/nodes/majority_vote_aggregation.hpp>

#include <algorithm>
#include <cstdint>

#include <iris/core/node_registry.hpp>

namespace iris {

MajorityVoteAggregation::MajorityVoteAggregation(
    const std::unordered_map<std::string, std::string>& node_params) {
    if (auto it = node_params.find("consistency_threshold"); it != node_params.end())
        params_.consistency_threshold = std::stod(it->second);
    if (auto it = node_params.find("mask_threshold"); it != node_params.end())
        params_.mask_threshold = std::stod(it->second);
    if (auto it = node_params.find("use_inconsistent_bits"); it != node_params.end())
        params_.use_inconsistent_bits = (it->second == "true" || it->second == "1");
    if (auto it = node_params.find("inconsistent_bit_threshold"); it != node_params.end())
        params_.inconsistent_bit_threshold = std::stod(it->second);
}

Result<WeightedIrisTemplate> MajorityVoteAggregation::run(
    const AlignedTemplates& input) const {

    if (input.templates.empty()) {
        return make_error(ErrorCode::TemplateAggregationIncompatible,
                          "No templates to aggregate");
    }

    const size_t n_templates = input.templates.size();
    const size_t n_wavelets = input.templates[0].iris_codes.size();

    // Verify all templates have the same number of wavelet pairs
    for (size_t t = 1; t < n_templates; ++t) {
        if (input.templates[t].iris_codes.size() != n_wavelets) {
            return make_error(ErrorCode::TemplateAggregationIncompatible,
                              "Mismatched number of wavelet pairs");
        }
    }

    WeightedIrisTemplate result;
    result.iris_code_version = input.templates[0].iris_code_version;

    for (size_t w = 0; w < n_wavelets; ++w) {
        // Unpack all templates for this wavelet
        std::vector<cv::Mat> codes(n_templates);
        std::vector<cv::Mat> masks(n_templates);

        for (size_t t = 0; t < n_templates; ++t) {
            auto [code, mask] = input.templates[t].iris_codes[w].to_mat();
            codes[t] = code;
            masks[t] = mask;
        }

        const int rows = codes[0].rows;  // 16
        const int cols = codes[0].cols;   // 512

        cv::Mat out_code(rows, cols, CV_8UC1, cv::Scalar(0));
        cv::Mat out_mask(rows, cols, CV_8UC1, cv::Scalar(0));
        cv::Mat out_weight(rows, cols, CV_64FC1, cv::Scalar(0.0));

        const double n_dbl = static_cast<double>(n_templates);

        for (int r = 0; r < rows; ++r) {
            auto* code_row = out_code.ptr<uint8_t>(r);
            auto* mask_row = out_mask.ptr<uint8_t>(r);
            auto* weight_row = out_weight.ptr<double>(r);

            for (int c = 0; c < cols; ++c) {
                // Count templates with valid mask at this position
                int mask_count = 0;
                int vote_count = 0;

                for (size_t t = 0; t < n_templates; ++t) {
                    if (masks[t].at<uint8_t>(r, c) != 0) {
                        ++mask_count;
                        if (codes[t].at<uint8_t>(r, c) != 0) {
                            ++vote_count;
                        }
                    }
                }

                // Check mask threshold
                if (static_cast<double>(mask_count) / n_dbl < params_.mask_threshold) {
                    continue;  // output remains 0
                }

                if (mask_count == 0) continue;

                // Majority vote
                const bool majority = vote_count * 2 > mask_count;
                code_row[c] = majority ? 1 : 0;

                // Consistency
                const int agree = majority ? vote_count : (mask_count - vote_count);
                const double consistency = static_cast<double>(agree)
                                         / static_cast<double>(mask_count);

                if (consistency >= params_.consistency_threshold) {
                    mask_row[c] = 1;
                    weight_row[c] = consistency;
                } else if (params_.use_inconsistent_bits) {
                    mask_row[c] = 1;
                    weight_row[c] = params_.inconsistent_bit_threshold;
                }
                // else: mask stays 0, weight stays 0
            }
        }

        result.iris_codes.push_back(PackedIrisCode::from_mat(out_code, out_mask));
        result.mask_codes.push_back(PackedIrisCode::from_mat(out_mask, out_mask));
        result.weights.push_back(std::move(out_weight));
    }

    return result;
}

IRIS_REGISTER_NODE("iris.MajorityVoteAggregation", MajorityVoteAggregation);

}  // namespace iris
