#include <iris/pipeline/pipeline_executor.hpp>

#include <iris/pipeline/callback_dispatcher.hpp>
#include <iris/pipeline/node_dispatcher.hpp>

#include <future>
#include <mutex>

namespace iris {

Result<void> PipelineExecutor::execute_node(
    const PipelineNode& node,
    PipelineContext& ctx) {
    // Look up dispatch function
    auto disp = lookup_dispatch(node.config.class_name);
    if (!disp) return std::unexpected(disp.error());

    // Run the algorithm
    auto result = (*disp)(node.algorithm, node.config, ctx);
    if (!result) return std::unexpected(result.error());

    // Run callback validators
    for (size_t i = 0; i < node.validators.size(); ++i) {
        const auto& cb = node.config.callbacks[i];
        auto cb_disp = lookup_callback_dispatch(cb.class_name);
        if (!cb_disp) return std::unexpected(cb_disp.error());

        // The callback needs the node's output. For pair-producing nodes,
        // the callback validates the primary output. For single-output nodes,
        // the output is stored under the node name.
        const std::any* output = ctx.get_raw(node.config.name);

        // For pair-producing nodes, the output is stored under :0 and :1.
        // Callbacks need the primary output. Try the node name first,
        // then fall back to :0 if the node produces pairs.
        if (!output) {
            output = ctx.get_raw(node.config.name + ":0");
        }

        if (!output) {
            return make_error(ErrorCode::ConfigInvalid,
                              "No output found for callback on node: "
                                  + node.config.name);
        }

        auto cb_result = (*cb_disp)(node.validators[i], *output);
        if (!cb_result) return std::unexpected(cb_result.error());
    }

    return {};
}

Result<void> PipelineExecutor::execute(
    const PipelineDAG& dag,
    PipelineContext& ctx,
    const Options& opts) {
    errors_.clear();

    for (const auto& level : dag.levels()) {
        // Check cancellation
        if (opts.token && opts.token->is_cancelled()) {
            return make_error(ErrorCode::Cancelled, "Pipeline cancelled");
        }

        if (level.node_names.size() == 1) {
            // Single node — execute directly
            auto result = execute_node(dag.node(level.node_names[0]), ctx);
            if (!result) {
                if (opts.strategy == ErrorStrategy::Store) {
                    errors_.push_back(result.error());
                } else {
                    return result;
                }
            }
        } else {
            // Multiple nodes — execute in parallel
            std::mutex error_mutex;
            std::vector<IrisError> level_errors;

            std::vector<std::future<void>> futures;
            futures.reserve(level.node_names.size());

            for (const auto& name : level.node_names) {
                futures.push_back(pool_.submit([&, &node_name = name]() {
                    auto result = execute_node(dag.node(node_name), ctx);
                    if (!result) {
                        std::lock_guard lock(error_mutex);
                        level_errors.push_back(result.error());
                    }
                }));
            }

            // Wait for all nodes in this level
            for (auto& f : futures) {
                f.get();
            }

            if (!level_errors.empty()) {
                if (opts.strategy == ErrorStrategy::Store) {
                    for (auto& e : level_errors) {
                        errors_.push_back(std::move(e));
                    }
                } else {
                    return std::unexpected(level_errors.front());
                }
            }
        }
    }

    return {};
}

}  // namespace iris
