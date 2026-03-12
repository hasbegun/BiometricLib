#include <gtest/gtest.h>

#include <any>
#include <string>

#include <eyetrack/core/node_registry.hpp>

namespace {

using namespace eyetrack;

// A simple test node class
struct DummyNode {
    int value = 0;
    explicit DummyNode(const NodeParams& params) {
        if (auto it = params.find("value"); it != params.end()) {
            value = std::stoi(it->second);
        }
    }
};

TEST(NodeRegistry, register_and_create_node) {
    auto& registry = NodeRegistry::instance();
    registry.register_node("eyetrack.DummyNode",
        [](const NodeParams& params) -> std::any { return DummyNode(params); });

    ASSERT_TRUE(registry.has("eyetrack.DummyNode"));

    auto factory_result = registry.lookup("eyetrack.DummyNode");
    ASSERT_TRUE(factory_result.has_value());

    auto node = factory_result.value()({{"value", "42"}});
    auto dummy = std::any_cast<DummyNode>(node);
    EXPECT_EQ(dummy.value, 42);
}

TEST(NodeRegistry, unknown_node_returns_error) {
    auto& registry = NodeRegistry::instance();
    auto result = registry.lookup("eyetrack.NonExistentNode");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ConfigInvalid);
}

TEST(NodeRegistry, duplicate_registration_overwrites) {
    auto& registry = NodeRegistry::instance();

    registry.register_node("eyetrack.OverwriteTest",
        [](const NodeParams&) -> std::any { return 1; });
    registry.register_node("eyetrack.OverwriteTest",
        [](const NodeParams&) -> std::any { return 2; });

    auto factory_result = registry.lookup("eyetrack.OverwriteTest");
    ASSERT_TRUE(factory_result.has_value());

    auto val = std::any_cast<int>(factory_result.value()({}));
    EXPECT_EQ(val, 2);
}

// Test node registered via macro (in this TU)
struct MacroRegisteredNode {
    explicit MacroRegisteredNode(const NodeParams&) {}
};

EYETRACK_REGISTER_NODE("eyetrack.MacroRegisteredNode", MacroRegisteredNode);

TEST(NodeRegistry, macro_registration) {
    auto& registry = NodeRegistry::instance();
    EXPECT_TRUE(registry.has("eyetrack.MacroRegisteredNode"));

    auto factory_result = registry.lookup("eyetrack.MacroRegisteredNode");
    ASSERT_TRUE(factory_result.has_value());
}

}  // namespace
