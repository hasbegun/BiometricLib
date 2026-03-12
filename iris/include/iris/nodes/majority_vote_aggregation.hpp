#pragma once

#include <string_view>
#include <unordered_map>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Aggregate multiple aligned templates into a single weighted template
/// using majority voting.
///
/// For each bit position:
///   1. Count templates with valid mask → mask_count
///   2. If mask_count / N < mask_threshold → output mask = 0
///   3. Majority vote among valid templates
///   4. Consistency = max(votes, mask_count - votes) / mask_count
///   5. If consistency >= consistency_threshold → weight = consistency
///   6. Else if use_inconsistent_bits → weight = inconsistent_bit_threshold
///   7. Else → mask out the bit
class MajorityVoteAggregation : public Algorithm<MajorityVoteAggregation> {
public:
    static constexpr std::string_view kName = "MajorityVoteAggregation";

    struct Params {
        double consistency_threshold = 0.75;
        double mask_threshold = 0.01;
        bool use_inconsistent_bits = true;
        double inconsistent_bit_threshold = 0.4;
    };

    MajorityVoteAggregation() = default;
    explicit MajorityVoteAggregation(const Params& params) : params_(params) {}
    explicit MajorityVoteAggregation(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<WeightedIrisTemplate> run(const AlignedTemplates& input) const;

    const Params& params() const { return params_; }

private:
    Params params_;
};

}  // namespace iris
