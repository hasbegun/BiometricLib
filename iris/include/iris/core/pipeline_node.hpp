#pragma once

#include <any>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <iris/core/error.hpp>

namespace iris {

/// Type-erased pipeline node interface for DAG execution.
///
/// Each node wraps a concrete Algorithm<Derived> and provides a uniform
/// execute() method that operates on a shared context map (name -> std::any).
class PipelineNodeBase {
public:
    virtual ~PipelineNodeBase() = default;

    /// Execute this node. Reads inputs from `context`, writes output to `context`.
    virtual Result<void> execute(
        std::unordered_map<std::string, std::any>& context) const = 0;

    /// Node name (matches the YAML pipeline node name).
    [[nodiscard]] virtual std::string_view node_name() const noexcept = 0;

    /// Input dependencies (source_node names this node reads from).
    [[nodiscard]] virtual const std::vector<std::string>& dependencies() const noexcept = 0;
};

}  // namespace iris
