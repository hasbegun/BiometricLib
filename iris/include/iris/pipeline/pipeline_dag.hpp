#pragma once

#include <any>
#include <string>
#include <unordered_map>
#include <vector>

#include <iris/core/config.hpp>
#include <iris/core/error.hpp>

namespace iris {

/// A node in the execution DAG with its algorithm instance and metadata.
struct PipelineNode {
    NodeConfig config;
    std::any algorithm;
    std::vector<std::any> validators;  // callback validator instances
    std::vector<std::string> deps;     // source_node dependencies
};

/// A set of nodes that can execute in parallel (no inter-dependencies).
struct ExecutionLevel {
    std::vector<std::string> node_names;
};

/// Pipeline execution DAG built from PipelineConfig.
///
/// Validates config, instantiates algorithms via NodeRegistry, and computes
/// execution levels via Kahn's algorithm (BFS topological sort).
class PipelineDAG {
  public:
    /// Build a DAG from a pipeline config.
    /// Validates: all class_names exist, all source_node refs valid, no cycles.
    static Result<PipelineDAG> build(const PipelineConfig& config);

    /// Get the execution levels (topological order).
    [[nodiscard]] const std::vector<ExecutionLevel>& levels() const noexcept {
        return levels_;
    }

    /// Get a node by name.
    [[nodiscard]] const PipelineNode& node(const std::string& name) const {
        return nodes_.at(name);
    }

    /// Check if a node exists.
    [[nodiscard]] bool has_node(const std::string& name) const {
        return nodes_.contains(name);
    }

    /// All node names.
    [[nodiscard]] std::vector<std::string> node_names() const;

  private:
    std::unordered_map<std::string, PipelineNode> nodes_;
    std::vector<ExecutionLevel> levels_;
};

}  // namespace iris
