#pragma once

#include <vector>

#include <iris/core/error.hpp>
#include <iris/core/types.hpp>
#include <iris/nodes/majority_vote_aggregation.hpp>
#include <iris/nodes/template_alignment.hpp>

namespace iris {

/// Aggregates multiple IrisTemplates into a single WeightedIrisTemplate.
/// Performs alignment (rotation normalization) followed by majority-vote fusion.
class AggregationPipeline {
  public:
    struct Params {
        HammingDistanceBasedAlignment::Params alignment;
        MajorityVoteAggregation::Params aggregation;
    };

    AggregationPipeline() = default;
    explicit AggregationPipeline(const Params& params)
        : aligner_(params.alignment), aggregator_(params.aggregation) {}

    /// Aggregate a set of templates.
    /// Requires at least 2 templates.
    Result<WeightedIrisTemplate> run(
        const std::vector<IrisTemplateWithId>& templates) const;

  private:
    HammingDistanceBasedAlignment aligner_;
    MajorityVoteAggregation aggregator_;
};

}  // namespace iris
