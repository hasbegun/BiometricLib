#include <iris/pipeline/pipeline_dag.hpp>

#include <algorithm>
#include <queue>
#include <set>
#include <sstream>

#include <iris/core/node_registry.hpp>
#include <iris/pipeline/callback_dispatcher.hpp>
#include <iris/pipeline/node_dispatcher.hpp>

namespace iris {

namespace {

/// Extract all dependency node names from a NodeConfig's inputs.
/// Handles multi-source inputs (comma-separated "name:index" pairs).
std::vector<std::string> extract_deps(const NodeConfig& config) {
    std::set<std::string> deps;  // unique dependencies
    for (const auto& input : config.inputs) {
        if (input.source_node == "input") continue;  // built-in input, not a dep

        // Check for multi-source format: "name:idx,name2"
        if (input.source_node.find(',') != std::string::npos) {
            std::istringstream stream(input.source_node);
            std::string token;
            while (std::getline(stream, token, ',')) {
                auto colon = token.find(':');
                if (colon != std::string::npos) {
                    deps.insert(token.substr(0, colon));
                } else {
                    deps.insert(token);
                }
            }
        } else {
            deps.insert(input.source_node);
        }
    }
    return {deps.begin(), deps.end()};
}

}  // namespace

Result<PipelineDAG> PipelineDAG::build(const PipelineConfig& config) {
    if (config.nodes.empty()) {
        return make_error(ErrorCode::ConfigInvalid, "Pipeline has no nodes");
    }

    PipelineDAG dag;
    auto& registry = NodeRegistry::instance();

    // Phase 1: Validate class_names and instantiate all algorithms
    for (const auto& node_config : config.nodes) {
        if (dag.nodes_.contains(node_config.name)) {
            return make_error(ErrorCode::ConfigInvalid,
                              "Duplicate node name: " + node_config.name);
        }

        // Check dispatch function exists
        auto disp = lookup_dispatch(node_config.class_name);
        if (!disp) {
            return make_error(ErrorCode::ConfigInvalid,
                              "No dispatch function for class_name: "
                                  + node_config.class_name
                                  + " (node: " + node_config.name + ")");
        }

        // Instantiate algorithm via registry
        auto factory = registry.lookup(node_config.class_name);
        if (!factory) {
            return make_error(ErrorCode::ConfigInvalid,
                              "Unknown class_name in registry: "
                                  + node_config.class_name
                                  + " (node: " + node_config.name + ")");
        }
        auto algorithm = (*factory)(node_config.params);

        // Instantiate callback validators
        std::vector<std::any> validators;
        for (const auto& cb : node_config.callbacks) {
            auto cb_factory = registry.lookup(cb.class_name);
            if (!cb_factory) {
                return make_error(ErrorCode::ConfigInvalid,
                                  "Unknown callback class_name: " + cb.class_name
                                      + " (node: " + node_config.name + ")");
            }
            validators.push_back((*cb_factory)(cb.params));
        }

        auto deps = extract_deps(node_config);

        dag.nodes_[node_config.name] = PipelineNode{
            .config = node_config,
            .algorithm = std::move(algorithm),
            .validators = std::move(validators),
            .deps = std::move(deps),
        };
    }

    // Phase 2: Validate all dependencies exist
    for (const auto& [name, node] : dag.nodes_) {
        for (const auto& dep : node.deps) {
            if (!dag.nodes_.contains(dep)) {
                return make_error(ErrorCode::ConfigInvalid,
                                  "Node '" + name + "' depends on unknown node: " + dep);
            }
        }
    }

    // Phase 3: Topological sort via Kahn's algorithm
    std::unordered_map<std::string, size_t> in_degree;
    std::unordered_map<std::string, std::vector<std::string>> dependents;

    for (const auto& [name, node] : dag.nodes_) {
        in_degree[name] += 0;  // ensure entry exists
        for (const auto& dep : node.deps) {
            dependents[dep].push_back(name);
            in_degree[name]++;
        }
    }

    std::queue<std::string> ready;
    for (const auto& [name, degree] : in_degree) {
        if (degree == 0) ready.push(name);
    }

    size_t processed = 0;
    while (!ready.empty()) {
        ExecutionLevel level;
        auto level_size = ready.size();
        for (size_t i = 0; i < level_size; ++i) {
            auto name = ready.front();
            ready.pop();
            level.node_names.push_back(name);
            ++processed;

            for (const auto& dependent : dependents[name]) {
                if (--in_degree[dependent] == 0) {
                    ready.push(dependent);
                }
            }
        }
        dag.levels_.push_back(std::move(level));
    }

    if (processed != dag.nodes_.size()) {
        return make_error(ErrorCode::ConfigInvalid,
                          "Circular dependency detected in pipeline");
    }

    return dag;
}

std::vector<std::string> PipelineDAG::node_names() const {
    std::vector<std::string> names;
    names.reserve(nodes_.size());
    for (const auto& [name, _] : nodes_) {
        names.push_back(name);
    }
    return names;
}

}  // namespace iris
