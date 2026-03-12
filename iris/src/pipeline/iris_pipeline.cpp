#include <iris/pipeline/iris_pipeline.hpp>

namespace iris {

Result<IrisPipeline> IrisPipeline::from_config(const std::filesystem::path& path) {
    auto config = load_pipeline_config(path);
    if (!config) return std::unexpected(config.error());
    return from_config(*config);
}

Result<IrisPipeline> IrisPipeline::from_config(const PipelineConfig& config) {
    auto dag = PipelineDAG::build(config);
    if (!dag) return std::unexpected(dag.error());

    auto pool = std::make_unique<ThreadPool>();
    auto dag_ptr = std::make_unique<PipelineDAG>(std::move(*dag));

    return IrisPipeline(std::move(dag_ptr), std::move(pool));
}

Result<PipelineOutput> IrisPipeline::run(
    const IRImage& image,
    CancellationToken* token) {
    PipelineContext ctx;
    return run(image, ctx, token);
}

Result<PipelineOutput> IrisPipeline::run(
    const IRImage& image,
    PipelineContext& ctx,
    CancellationToken* token) {
    ctx.set("input", image);

    PipelineExecutor executor(*pool_);
    PipelineExecutor::Options opts;
    opts.strategy = PipelineExecutor::ErrorStrategy::Raise;
    opts.token = token;

    auto result = executor.execute(*dag_, ctx, opts);
    if (!result) {
        return PipelineOutput{.iris_template = std::nullopt, .error = result.error()};
    }

    // Extract the final IrisTemplate from the "encoder" node
    auto tmpl = ctx.get<IrisTemplate>("encoder");
    if (tmpl) {
        return PipelineOutput{.iris_template = **tmpl, .error = std::nullopt};
    }

    // If no encoder output, return output without template
    return PipelineOutput{.iris_template = std::nullopt, .error = std::nullopt};
}

}  // namespace iris
