#include <iris/pipeline/aggregation_pipeline.hpp>

namespace iris {

Result<WeightedIrisTemplate> AggregationPipeline::run(
    const std::vector<IrisTemplateWithId>& templates) const {
    if (templates.size() < 2) {
        return make_error(ErrorCode::ConfigInvalid,
                          "AggregationPipeline requires at least 2 templates, got "
                              + std::to_string(templates.size()));
    }

    auto aligned = aligner_.run(templates);
    if (!aligned) return std::unexpected(aligned.error());

    return aggregator_.run(*aligned);
}

}  // namespace iris
