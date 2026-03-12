#include <iris/pipeline/multiframe_pipeline.hpp>

#include <future>
#include <mutex>

namespace iris {

Result<MultiframePipeline> MultiframePipeline::from_config(
    const PipelineConfig& config,
    const Params& params) {
    auto pipeline = IrisPipeline::from_config(config);
    if (!pipeline) return std::unexpected(pipeline.error());

    AggregationPipeline aggregator(params.aggregation);

    return MultiframePipeline(std::move(*pipeline), std::move(aggregator), params);
}

Result<WeightedIrisTemplate> MultiframePipeline::run(
    const std::vector<IRImage>& images,
    CancellationToken* token) {
    if (images.size() < params_.min_successful) {
        return make_error(ErrorCode::ConfigInvalid,
                          "Need at least " + std::to_string(params_.min_successful)
                              + " images, got " + std::to_string(images.size()));
    }

    // Process each image through the pipeline
    std::mutex results_mutex;
    std::vector<IrisTemplateWithId> successful;
    std::vector<IrisError> failures;

    // Run sequentially (each pipeline run uses the internal thread pool)
    for (size_t i = 0; i < images.size(); ++i) {
        if (token && token->is_cancelled()) {
            return make_error(ErrorCode::Cancelled, "Multiframe pipeline cancelled");
        }

        auto output = pipeline_.run(images[i], token);
        if (!output) {
            failures.push_back(output.error());
            continue;
        }

        if (output->error.has_value()) {
            failures.push_back(*output->error);
            continue;
        }

        if (output->iris_template.has_value()) {
            IrisTemplateWithId tmpl_with_id;
            tmpl_with_id.iris_codes = output->iris_template->iris_codes;
            tmpl_with_id.mask_codes = output->iris_template->mask_codes;
            tmpl_with_id.iris_code_version = output->iris_template->iris_code_version;
            tmpl_with_id.image_id = images[i].image_id.empty()
                                        ? std::to_string(i)
                                        : images[i].image_id;
            successful.push_back(std::move(tmpl_with_id));
        }
    }

    // Check if we have enough successful results
    if (successful.size() < params_.min_successful) {
        std::string msg = "Only " + std::to_string(successful.size())
                          + " of " + std::to_string(images.size())
                          + " images succeeded, need " + std::to_string(params_.min_successful);
        if (!failures.empty()) {
            msg += ". First error: " + failures.front().message;
        }
        return make_error(ErrorCode::PipelineFailed, msg);
    }

    return aggregator_.run(successful);
}

}  // namespace iris
