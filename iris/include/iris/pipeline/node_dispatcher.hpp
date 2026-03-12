#pragma once

#include <any>
#include <functional>
#include <string>
#include <unordered_map>

#include <iris/core/config.hpp>
#include <iris/core/error.hpp>
#include <iris/pipeline/pipeline_context.hpp>

namespace iris {

/// Signature for a dispatch function that bridges type-erased std::any
/// to a strongly-typed algorithm run() call.
using DispatchFn = std::function<Result<void>(
    const std::any& algorithm,
    const NodeConfig& config,
    PipelineContext& ctx)>;

/// Returns the global dispatch table mapping Python class_name → DispatchFn.
/// Covers all 22 pipeline.yaml algorithm nodes.
const std::unordered_map<std::string, DispatchFn>& dispatch_table();

/// Look up a dispatch function by class_name.
/// Returns ConfigInvalid error if not found.
Result<DispatchFn> lookup_dispatch(const std::string& class_name);

/// Helper: read a typed input from PipelineContext using NodeConfig::Input.
/// Handles indexed access (source_node + ":index" suffix).
template <typename T>
Result<const T*> read_input(const PipelineContext& ctx, const NodeConfig::Input& input) {
    std::string key = input.source_node;
    if (input.index >= 0) {
        key += ":" + std::to_string(input.index);
    }
    return ctx.get<T>(key);
}

}  // namespace iris
