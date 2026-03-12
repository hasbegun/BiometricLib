#pragma once

#include <memory>
#include <string>
#include <vector>

#include <eyetrack/core/config.hpp>
#include <eyetrack/core/error.hpp>
#include <eyetrack/core/pipeline_node.hpp>

namespace eyetrack {

/// DAG-based pipeline definition.
///
/// Constructs a topologically sorted list of PipelineNodeBase instances
/// from a PipelineConfig. Nodes are looked up via the NodeRegistry.
class EyetrackPipeline {
public:
    EyetrackPipeline() = default;

    /// Build the pipeline from configuration.
    /// Resolves node names via the NodeRegistry and performs topological sort.
    [[nodiscard]] static Result<EyetrackPipeline> from_config(
        const PipelineConfig& config);

    /// Get the sorted list of pipeline nodes (execution order).
    [[nodiscard]] const std::vector<std::shared_ptr<PipelineNodeBase>>&
    nodes() const noexcept {
        return nodes_;
    }

    /// Get pipeline name.
    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    /// Number of nodes in the pipeline.
    [[nodiscard]] size_t size() const noexcept { return nodes_.size(); }

    /// Check if pipeline is empty.
    [[nodiscard]] bool empty() const noexcept { return nodes_.empty(); }

private:
    std::string name_;
    std::vector<std::shared_ptr<PipelineNodeBase>> nodes_;
};

}  // namespace eyetrack
