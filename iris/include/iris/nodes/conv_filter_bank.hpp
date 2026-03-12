#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <opencv2/core.hpp>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>
#include <iris/nodes/gabor_filter.hpp>
#include <iris/nodes/regular_probe_schema.hpp>

namespace iris {

/// Convolutional filter bank for iris feature extraction.
///
/// Applies a set of Gabor filters at probe locations on a normalized iris
/// image, producing complex-valued filter responses and mask reliability
/// weights. Output feeds into FragileBitRefinement → IrisEncoder.
class ConvFilterBank : public Algorithm<ConvFilterBank> {
public:
    static constexpr std::string_view kName = "ConvFilterBank";

    struct Params {
        std::string iris_code_version = "v0.1";
        bool mask_is_duplicated = true;
    };

    /// Default: two Gabor filters matching the default pipeline.yaml.
    ConvFilterBank();
    explicit ConvFilterBank(Params params,
                            std::vector<GaborFilter> filters,
                            std::vector<RegularProbeSchema> schemas);
    explicit ConvFilterBank(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<IrisFilterResponse> run(const NormalizedIris& input) const;

    const Params& params() const noexcept { return params_; }
    size_t num_filters() const noexcept { return filters_.size(); }

private:
    Params params_;
    std::vector<GaborFilter> filters_;
    std::vector<RegularProbeSchema> schemas_;

    /// Convolve one filter-schema pair against the normalized iris.
    IrisFilterResponse::Response convolve(const GaborFilter& filter,
                                          const RegularProbeSchema& schema,
                                          const NormalizedIris& input) const;

    /// Pad image: zero-pad vertically, circular wrap horizontally.
    static cv::Mat polar_pad(const cv::Mat& img, int pad_cols);
};

}  // namespace iris
