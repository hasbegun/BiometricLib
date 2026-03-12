#pragma once

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <eyetrack/core/error.hpp>
#include <eyetrack/core/pipeline_node.hpp>

namespace eyetrack {

/// Factory signature: takes params map, returns a type-erased algorithm.
using NodeParams = std::unordered_map<std::string, std::string>;
using NodeFactory = std::function<std::any(const NodeParams&)>;

/// Factory that creates a PipelineNodeBase for pipeline orchestration.
using PipelineNodeFactory =
    std::function<std::unique_ptr<PipelineNodeBase>(const NodeParams&)>;

/// Registry mapping class name strings to C++ node factories.
///
/// Singleton. Nodes auto-register at static init time via EYETRACK_REGISTER_NODE.
class NodeRegistry {
public:
    static NodeRegistry& instance();

    /// Register a node factory under a class name.
    void register_node(std::string class_name, NodeFactory factory);

    /// Register a pipeline node factory (creates unique_ptr<PipelineNodeBase>).
    void register_pipeline_node(std::string class_name,
                                PipelineNodeFactory factory);

    /// Look up a factory by class name.
    [[nodiscard]] Result<NodeFactory> lookup(std::string_view class_name) const;

    /// Look up a pipeline node factory by class name.
    [[nodiscard]] Result<PipelineNodeFactory> lookup_pipeline(
        std::string_view class_name) const;

    /// Check whether a class name is registered.
    [[nodiscard]] bool has(std::string_view class_name) const;

    /// All registered class names (for diagnostics).
    [[nodiscard]] std::vector<std::string> registered_names() const;

private:
    NodeRegistry() = default;
    std::unordered_map<std::string, NodeFactory> factories_;
    std::unordered_map<std::string, PipelineNodeFactory> pipeline_factories_;
};

/// Auto-registration helper.
namespace detail {

struct NodeRegistrar {
    NodeRegistrar(std::string class_name, NodeFactory factory) {
        NodeRegistry::instance().register_node(std::move(class_name), std::move(factory));
    }
};

/// Conditionally registers a pipeline node factory only if CppClass
/// inherits from PipelineNodeBase.
template <typename CppClass>
void maybe_register_pipeline(const std::string& class_name) {
    if constexpr (std::is_base_of_v<PipelineNodeBase, CppClass>) {
        NodeRegistry::instance().register_pipeline_node(
            class_name,
            [](const NodeParams& params)
                -> std::unique_ptr<PipelineNodeBase> {
                return std::make_unique<CppClass>(params);
            });
    }
}

}  // namespace detail

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EYETRACK_REGISTER_NODE(class_name_str, CppClass)                            \
    static ::eyetrack::detail::NodeRegistrar registrar_##CppClass(                  \
        class_name_str, [](const ::eyetrack::NodeParams& params) -> std::any {      \
            return CppClass(params);                                                \
        });                                                                         \
    static const int registrar_pipeline_##CppClass = [] {                           \
        ::eyetrack::detail::maybe_register_pipeline<CppClass>(class_name_str);      \
        return 0;                                                                   \
    }()

}  // namespace eyetrack
