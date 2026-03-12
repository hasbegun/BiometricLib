#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include <eyetrack/core/cancellation.hpp>
#include <eyetrack/core/config.hpp>
#include <eyetrack/core/node_registry.hpp>
#include <eyetrack/pipeline/eyetrack_pipeline.hpp>
#include <eyetrack/pipeline/pipeline_context.hpp>
#include <eyetrack/pipeline/pipeline_executor.hpp>

namespace {

// Mock node that records execution order.
class MockNodeA : public eyetrack::PipelineNodeBase {
public:
    explicit MockNodeA(
        const eyetrack::NodeParams& /*params*/) {}

    eyetrack::Result<void> execute(
        std::unordered_map<std::string, std::any>& context) const override {
        // Record that A executed
        int order = 0;
        if (auto it = context.find("exec_count"); it != context.end()) {
            order = std::any_cast<int>(it->second);
        }
        context["exec_count"] = order + 1;
        context["node_a_order"] = order;
        return {};
    }

    [[nodiscard]] std::string_view node_name() const noexcept override {
        return "test.MockNodeA";
    }

    [[nodiscard]] const std::vector<std::string>& dependencies()
        const noexcept override {
        static const std::vector<std::string> deps;
        return deps;
    }
};

class MockNodeB : public eyetrack::PipelineNodeBase {
public:
    explicit MockNodeB(
        const eyetrack::NodeParams& /*params*/) {}

    eyetrack::Result<void> execute(
        std::unordered_map<std::string, std::any>& context) const override {
        int order = 0;
        if (auto it = context.find("exec_count"); it != context.end()) {
            order = std::any_cast<int>(it->second);
        }
        context["exec_count"] = order + 1;
        context["node_b_order"] = order;
        return {};
    }

    [[nodiscard]] std::string_view node_name() const noexcept override {
        return "test.MockNodeB";
    }

    [[nodiscard]] const std::vector<std::string>& dependencies()
        const noexcept override {
        static const std::vector<std::string> deps = {"test.MockNodeA"};
        return deps;
    }
};

// A node that sleeps briefly (for cancellation testing).
class SlowNode : public eyetrack::PipelineNodeBase {
public:
    explicit SlowNode(const eyetrack::NodeParams& /*params*/) {}

    eyetrack::Result<void> execute(
        std::unordered_map<std::string, std::any>& context) const override {
        context["slow_executed"] = true;
        return {};
    }

    [[nodiscard]] std::string_view node_name() const noexcept override {
        return "test.SlowNode";
    }

    [[nodiscard]] const std::vector<std::string>& dependencies()
        const noexcept override {
        static const std::vector<std::string> deps = {"test.MockNodeB"};
        return deps;
    }
};

// Register mock nodes
EYETRACK_REGISTER_NODE("test.MockNodeA", MockNodeA);
EYETRACK_REGISTER_NODE("test.MockNodeB", MockNodeB);
EYETRACK_REGISTER_NODE("test.SlowNode", SlowNode);

}  // namespace

TEST(PipelineExecutor, sync_mode_runs_sequentially) {
    eyetrack::PipelineConfig config;
    config.pipeline_name = "test_sync";
    config.nodes = {"test.MockNodeA", "test.MockNodeB"};

    auto pipeline = eyetrack::EyetrackPipeline::from_config(config);
    ASSERT_TRUE(pipeline.has_value()) << pipeline.error().message;

    auto shared_pipeline =
        std::make_shared<eyetrack::EyetrackPipeline>(std::move(*pipeline));

    eyetrack::PipelineExecutor::Config exec_cfg;
    exec_cfg.mode = eyetrack::ExecutionMode::Sync;
    eyetrack::PipelineExecutor executor(exec_cfg);
    executor.set_pipeline(shared_pipeline);

    eyetrack::PipelineContext ctx;
    auto result = executor.execute(ctx);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Verify both nodes executed
    auto& raw = ctx.raw();
    ASSERT_TRUE(raw.contains("node_a_order"));
    ASSERT_TRUE(raw.contains("node_b_order"));

    int a_order = std::any_cast<int>(raw.at("node_a_order"));
    int b_order = std::any_cast<int>(raw.at("node_b_order"));

    // A should execute before B (A has no deps, B depends on A)
    EXPECT_LT(a_order, b_order);
}

TEST(PipelineExecutor, async_mode_runs_with_thread_pool) {
    // Async mode still executes nodes in dependency order
    eyetrack::PipelineConfig config;
    config.pipeline_name = "test_async";
    config.nodes = {"test.MockNodeA", "test.MockNodeB"};

    auto pipeline = eyetrack::EyetrackPipeline::from_config(config);
    ASSERT_TRUE(pipeline.has_value()) << pipeline.error().message;

    auto shared_pipeline =
        std::make_shared<eyetrack::EyetrackPipeline>(std::move(*pipeline));

    eyetrack::PipelineExecutor::Config exec_cfg;
    exec_cfg.mode = eyetrack::ExecutionMode::Async;
    eyetrack::PipelineExecutor executor(exec_cfg);
    executor.set_pipeline(shared_pipeline);

    eyetrack::PipelineContext ctx;
    auto result = executor.execute(ctx);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Verify execution completed
    EXPECT_TRUE(ctx.raw().contains("node_a_order"));
    EXPECT_TRUE(ctx.raw().contains("node_b_order"));
}

TEST(PipelineExecutor, frame_rate_adaptation_skips_frames) {
    eyetrack::PipelineExecutor::Config cfg;
    cfg.target_fps = 30;  // ~33ms per frame
    eyetrack::PipelineExecutor executor(cfg);

    // Initially should not skip (no previous frame)
    EXPECT_FALSE(executor.should_skip_frame());

    // After marking a frame, should skip if called immediately
    executor.mark_frame_start();
    EXPECT_TRUE(executor.should_skip_frame());

    // After waiting long enough, should not skip
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    EXPECT_FALSE(executor.should_skip_frame());
}

TEST(PipelineExecutor, loads_config_from_yaml) {
    std::string yaml = R"(
pipeline_name: test_yaml_pipeline
target_fps: 60
nodes:
  - test.MockNodeA
  - test.MockNodeB
)";

    auto config = eyetrack::parse_pipeline_config(yaml);
    ASSERT_TRUE(config.has_value()) << config.error().message;

    EXPECT_EQ(config->pipeline_name, "test_yaml_pipeline");
    EXPECT_EQ(config->target_fps, 60U);
    EXPECT_EQ(config->nodes.size(), 2U);

    auto pipeline = eyetrack::EyetrackPipeline::from_config(*config);
    ASSERT_TRUE(pipeline.has_value()) << pipeline.error().message;
    EXPECT_EQ(pipeline->size(), 2U);
}

TEST(PipelineExecutor, empty_pipeline_returns_error) {
    eyetrack::PipelineExecutor executor;

    eyetrack::PipelineContext ctx;
    auto result = executor.execute(ctx);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::ConfigInvalid);
}

TEST(PipelineExecutor, cancellation_stops_pipeline) {
    eyetrack::PipelineConfig config;
    config.pipeline_name = "test_cancel";
    config.nodes = {"test.MockNodeA", "test.MockNodeB", "test.SlowNode"};

    auto pipeline = eyetrack::EyetrackPipeline::from_config(config);
    ASSERT_TRUE(pipeline.has_value()) << pipeline.error().message;

    auto shared_pipeline =
        std::make_shared<eyetrack::EyetrackPipeline>(std::move(*pipeline));

    eyetrack::PipelineExecutor executor;
    executor.set_pipeline(shared_pipeline);

    // Cancel before execution starts
    eyetrack::CancellationToken token;
    token.cancel();

    eyetrack::PipelineContext ctx;
    auto result = executor.execute(ctx, token);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::Cancelled);

    // Verify no nodes executed
    EXPECT_FALSE(ctx.raw().contains("node_a_order"));
}
