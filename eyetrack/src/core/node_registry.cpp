#include <eyetrack/core/node_registry.hpp>

namespace eyetrack {

NodeRegistry& NodeRegistry::instance() {
    static NodeRegistry registry;
    return registry;
}

void NodeRegistry::register_node(std::string class_name, NodeFactory factory) {
    factories_[std::move(class_name)] = std::move(factory);
}

void NodeRegistry::register_pipeline_node(std::string class_name,
                                           PipelineNodeFactory factory) {
    pipeline_factories_[std::move(class_name)] = std::move(factory);
}

Result<NodeFactory> NodeRegistry::lookup(std::string_view class_name) const {
    auto it = factories_.find(std::string(class_name));
    if (it == factories_.end()) {
        return make_error(
            ErrorCode::ConfigInvalid,
            "Unknown node class_name",
            std::string(class_name));
    }
    return it->second;
}

Result<PipelineNodeFactory> NodeRegistry::lookup_pipeline(
    std::string_view class_name) const {
    auto it = pipeline_factories_.find(std::string(class_name));
    if (it == pipeline_factories_.end()) {
        return make_error(
            ErrorCode::ConfigInvalid,
            "Unknown pipeline node",
            std::string(class_name));
    }
    return it->second;
}

bool NodeRegistry::has(std::string_view class_name) const {
    return factories_.contains(std::string(class_name));
}

std::vector<std::string> NodeRegistry::registered_names() const {
    std::vector<std::string> names;
    names.reserve(factories_.size());
    for (const auto& [name, _] : factories_) {
        names.push_back(name);
    }
    return names;
}

}  // namespace eyetrack
