#include <eyetrack/pipeline/eyetrack_pipeline.hpp>

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include <eyetrack/core/node_registry.hpp>

namespace eyetrack {

namespace {

// Map common YAML node names to registry class names.
std::string resolve_node_name(const std::string& name) {
    static const std::unordered_map<std::string, std::string> kAliases = {
        {"face_detection", "eyetrack.FaceDetector"},
        {"eye_landmark", "eyetrack.EyeExtractor"},
        {"pupil_detection", "eyetrack.PupilDetector"},
        {"blink_detection", "eyetrack.BlinkDetector"},
        {"gaze_estimation", "eyetrack.GazeEstimator"},
        {"gaze_smoothing", "eyetrack.GazeSmoother"},
        {"calibration", "eyetrack.CalibrationManager"},
        {"preprocessor", "eyetrack.Preprocessor"},
    };

    if (auto it = kAliases.find(name); it != kAliases.end()) {
        return it->second;
    }
    // Assume it's already a fully qualified class name
    return name;
}

// Topological sort using Kahn's algorithm.
// Returns sorted node indices or error if cycle detected.
Result<std::vector<size_t>> topological_sort(
    const std::vector<std::shared_ptr<PipelineNodeBase>>& nodes) {

    size_t n = nodes.size();

    // Build name -> index map
    std::unordered_map<std::string, size_t> name_to_idx;
    for (size_t i = 0; i < n; ++i) {
        name_to_idx[std::string(nodes[i]->node_name())] = i;
    }

    // Build adjacency and in-degree
    std::vector<std::vector<size_t>> adj(n);
    std::vector<size_t> in_degree(n, 0);

    for (size_t i = 0; i < n; ++i) {
        for (const auto& dep : nodes[i]->dependencies()) {
            auto it = name_to_idx.find(dep);
            if (it != name_to_idx.end()) {
                adj[it->second].push_back(i);
                in_degree[i]++;
            }
            // Dependencies not in the pipeline are ignored
            // (they may have been handled externally)
        }
    }

    // Kahn's algorithm
    std::vector<size_t> queue;
    for (size_t i = 0; i < n; ++i) {
        if (in_degree[i] == 0) queue.push_back(i);
    }

    std::vector<size_t> sorted;
    sorted.reserve(n);

    while (!queue.empty()) {
        size_t u = queue.back();
        queue.pop_back();
        sorted.push_back(u);

        for (size_t v : adj[u]) {
            if (--in_degree[v] == 0) {
                queue.push_back(v);
            }
        }
    }

    if (sorted.size() != n) {
        return make_error(ErrorCode::ConfigInvalid,
                          "Circular dependency detected in pipeline");
    }

    return sorted;
}

}  // namespace

Result<EyetrackPipeline> EyetrackPipeline::from_config(
    const PipelineConfig& config) {

    if (config.nodes.empty()) {
        return make_error(ErrorCode::ConfigInvalid,
                          "Pipeline has no nodes configured");
    }

    auto& registry = NodeRegistry::instance();
    std::vector<std::shared_ptr<PipelineNodeBase>> nodes;
    nodes.reserve(config.nodes.size());

    NodeParams params;

    for (const auto& node_name : config.nodes) {
        auto class_name = resolve_node_name(node_name);
        auto factory = registry.lookup_pipeline(class_name);
        if (!factory.has_value()) {
            return make_error(ErrorCode::ConfigInvalid,
                              "Unknown pipeline node: " + node_name,
                              "Resolved to: " + class_name);
        }
        auto node = factory.value()(params);
        nodes.push_back(std::move(node));
    }

    // Topological sort
    auto sorted_indices = topological_sort(nodes);
    if (!sorted_indices.has_value()) {
        return std::unexpected(sorted_indices.error());
    }

    EyetrackPipeline pipeline;
    pipeline.name_ = config.pipeline_name;
    pipeline.nodes_.reserve(nodes.size());
    for (size_t idx : sorted_indices.value()) {
        pipeline.nodes_.push_back(std::move(nodes[idx]));
    }

    return pipeline;
}

}  // namespace eyetrack
