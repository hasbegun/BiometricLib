#include <iris/core/node_registry.hpp>

#include <any>

#include <gtest/gtest.h>

using namespace iris;

TEST(NodeRegistry, LookupUnknownFails) {
    auto result = NodeRegistry::instance().lookup("iris.DoesNotExist");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ConfigInvalid);
}

TEST(NodeRegistry, RegisterAndLookup) {
    // Register a dummy node
    NodeRegistry::instance().register_node(
        "iris.test.DummyNode",
        [](const NodeParams&) -> std::any { return 42; });

    EXPECT_TRUE(NodeRegistry::instance().has("iris.test.DummyNode"));

    auto result = NodeRegistry::instance().lookup("iris.test.DummyNode");
    EXPECT_TRUE(result.has_value());
}

TEST(NodeRegistry, IrisEncoderIsRegistered) {
    // The encoder registers itself via IRIS_REGISTER_NODE in encoder.cpp.
    // This test verifies auto-registration works (requires encoder.cpp to be linked).
    EXPECT_TRUE(NodeRegistry::instance().has("iris.IrisEncoder"));
}

TEST(NodeRegistry, RegisteredNamesIncludesEncoder) {
    auto names = NodeRegistry::instance().registered_names();
    bool found = false;
    for (const auto& n : names) {
        if (n == "iris.IrisEncoder") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "iris.IrisEncoder not in registered names";
}
