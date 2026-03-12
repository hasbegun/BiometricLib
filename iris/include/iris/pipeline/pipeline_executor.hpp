#pragma once

#include <iris/core/cancellation.hpp>
#include <iris/core/error.hpp>
#include <iris/core/thread_pool.hpp>
#include <iris/pipeline/pipeline_context.hpp>
#include <iris/pipeline/pipeline_dag.hpp>

namespace iris {

/// Executes a PipelineDAG level-by-level with optional parallelism.
class PipelineExecutor {
  public:
    /// How to handle node/callback errors.
    enum class ErrorStrategy { Raise, Store };

    struct Options {
        ErrorStrategy strategy = ErrorStrategy::Raise;
        CancellationToken* token = nullptr;
    };

    explicit PipelineExecutor(ThreadPool& pool) : pool_(pool) {}

    /// Execute all DAG levels in order. Nodes within a level run in parallel.
    Result<void> execute(
        const PipelineDAG& dag,
        PipelineContext& ctx,
        const Options& opts);

    /// Execute with default options.
    Result<void> execute(
        const PipelineDAG& dag,
        PipelineContext& ctx) {
        Options opts;
        return execute(dag, ctx, opts);
    }

    /// Errors collected when using ErrorStrategy::Store.
    [[nodiscard]] const std::vector<IrisError>& collected_errors() const noexcept {
        return errors_;
    }

  private:
    Result<void> execute_node(
        const PipelineNode& node,
        PipelineContext& ctx);

    ThreadPool& pool_;
    std::vector<IrisError> errors_;
};

}  // namespace iris
