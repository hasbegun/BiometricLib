#pragma once

#include <any>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <iris/core/error.hpp>

namespace iris {

/// Factory signature: takes YAML params (as key-value map), returns a type-erased algorithm.
/// The concrete type is recovered by the pipeline executor in Phase 6.
using NodeParams = std::unordered_map<std::string, std::string>;
using NodeFactory = std::function<std::any(const NodeParams&)>;

/// Registry mapping Python class_name strings to C++ node factories.
///
/// Singleton. Nodes auto-register at static init time via IRIS_REGISTER_NODE.
class NodeRegistry {
public:
    static NodeRegistry& instance();

    /// Register a node factory under a Python class name.
    void register_node(std::string class_name, NodeFactory factory);

    /// Look up a factory by Python class name.
    [[nodiscard]] Result<NodeFactory> lookup(std::string_view class_name) const;

    /// Check whether a class name is registered.
    [[nodiscard]] bool has(std::string_view class_name) const;

    /// All registered class names (for diagnostics).
    [[nodiscard]] std::vector<std::string> registered_names() const;

private:
    NodeRegistry() = default;
    std::unordered_map<std::string, NodeFactory> factories_;
};

/// Auto-registration helper. Place in a .cpp file:
///   IRIS_REGISTER_NODE("iris.IrisEncoder", IrisEncoder)
///
/// This creates a static object whose constructor calls
/// NodeRegistry::instance().register_node(...).
namespace detail {

struct NodeRegistrar {
    NodeRegistrar(std::string class_name, NodeFactory factory) {
        NodeRegistry::instance().register_node(std::move(class_name), std::move(factory));
    }
};

}  // namespace detail

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRIS_REGISTER_NODE(python_class_name, CppClass)                              \
    static ::iris::detail::NodeRegistrar registrar_##CppClass(                       \
        python_class_name, [](const ::iris::NodeParams& params) -> std::any {        \
            return CppClass(params);                                                 \
        })

/// Register an additional alias for the same C++ class.
/// `unique_tag` must be unique across all registrations to avoid symbol collisions.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRIS_REGISTER_NODE_ALIAS(python_class_name, CppClass, unique_tag)            \
    static ::iris::detail::NodeRegistrar registrar_alias_##unique_tag(               \
        python_class_name, [](const ::iris::NodeParams& params) -> std::any {        \
            return CppClass(params);                                                 \
        })

}  // namespace iris
