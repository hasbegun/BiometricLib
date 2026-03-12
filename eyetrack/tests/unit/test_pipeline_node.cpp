#include <gtest/gtest.h>

#include <any>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <eyetrack/core/pipeline_node.hpp>

namespace {

using namespace eyetrack;

// Mock pipeline node for testing the interface
class MockNode : public PipelineNodeBase {
public:
    Result<void> execute(
        std::unordered_map<std::string, std::any>& context) const override {
        context["mock_output"] = 42;
        return {};
    }

    [[nodiscard]] std::string_view node_name() const noexcept override {
        return "mock_node";
    }

    [[nodiscard]] const std::vector<std::string>& dependencies() const noexcept override {
        static const std::vector<std::string> deps = {"input"};
        return deps;
    }
};

TEST(PipelineNode, interface_methods_exist) {
    MockNode node;
    EXPECT_EQ(node.node_name(), "mock_node");
    EXPECT_EQ(node.dependencies().size(), 1U);
    EXPECT_EQ(node.dependencies()[0], "input");
}

TEST(PipelineNode, process_returns_result) {
    MockNode node;
    std::unordered_map<std::string, std::any> context;
    auto result = node.execute(context);
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(context.contains("mock_output"));
    EXPECT_EQ(std::any_cast<int>(context["mock_output"]), 42);
}

}  // namespace
