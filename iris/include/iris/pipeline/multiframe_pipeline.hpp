#pragma once

#include <cstddef>
#include <vector>

#include <iris/core/cancellation.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>
#include <iris/pipeline/aggregation_pipeline.hpp>
#include <iris/pipeline/iris_pipeline.hpp>

namespace iris {

/// Processes multiple IR images through the iris pipeline and aggregates
/// the resulting templates into a single weighted template.
class MultiframePipeline {
  public:
    struct Params {
        size_t min_successful = 2;
        AggregationPipeline::Params aggregation;
    };

    /// Build from a pipeline config and multiframe parameters.
    static Result<MultiframePipeline> from_config(
        const PipelineConfig& config,
        const Params& params);

    /// Build from a pipeline config with default parameters.
    static Result<MultiframePipeline> from_config(
        const PipelineConfig& config) {
        Params p;
        return from_config(config, p);
    }

    /// Process multiple images and aggregate.
    Result<WeightedIrisTemplate> run(
        const std::vector<IRImage>& images,
        CancellationToken* token = nullptr);

  private:
    MultiframePipeline(IrisPipeline pipeline,
                       AggregationPipeline aggregator,
                       Params params)
        : pipeline_(std::move(pipeline)),
          aggregator_(std::move(aggregator)),
          params_(std::move(params)) {}

    IrisPipeline pipeline_;
    AggregationPipeline aggregator_;
    Params params_;
};

}  // namespace iris
