#pragma once

#include <filesystem>
#include <memory>

#include <iris/core/cancellation.hpp>
#include <iris/core/config.hpp>
#include <iris/core/error.hpp>
#include <iris/core/thread_pool.hpp>
#include <iris/core/types.hpp>
#include <iris/pipeline/pipeline_context.hpp>
#include <iris/pipeline/pipeline_dag.hpp>
#include <iris/pipeline/pipeline_executor.hpp>

namespace iris {

/// High-level pipeline facade: load config → build DAG → execute.
class IrisPipeline {
  public:
    /// Build from a YAML config file.
    static Result<IrisPipeline> from_config(const std::filesystem::path& path);

    /// Build from an already-parsed PipelineConfig.
    static Result<IrisPipeline> from_config(const PipelineConfig& config);

    /// Run the full pipeline on an IR image.
    Result<PipelineOutput> run(
        const IRImage& image,
        CancellationToken* token = nullptr);

    /// Run the full pipeline, exposing the PipelineContext for intermediate
    /// result inspection (debugging / comparison).
    Result<PipelineOutput> run(
        const IRImage& image,
        PipelineContext& ctx,
        CancellationToken* token = nullptr);

    /// Access the underlying DAG (for inspection/testing).
    [[nodiscard]] const PipelineDAG& dag() const noexcept { return *dag_; }

  private:
    IrisPipeline(std::unique_ptr<PipelineDAG> dag, std::unique_ptr<ThreadPool> pool)
        : dag_(std::move(dag)), pool_(std::move(pool)) {}

    std::unique_ptr<PipelineDAG> dag_;
    std::unique_ptr<ThreadPool> pool_;
};

}  // namespace iris
