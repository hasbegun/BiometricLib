#include <iris/pipeline/iris_pipeline.hpp>

#include <iris/core/cancellation.hpp>
#include <iris/core/config.hpp>
#include <iris/core/types.hpp>

#include <gtest/gtest.h>

namespace iris::test {

// --- Helpers ---

/// Build a simple single-node config (specular reflection detection).
/// Uses a node with a trivial constructor (no model loading).
PipelineConfig make_mini_config() {
    PipelineConfig config;
    config.pipeline_name = "test";
    config.iris_version = "1.0";

    NodeConfig a;
    a.name = "specular";
    a.class_name = "iris.SpecularReflectionDetection";
    a.inputs = {{.name = "ir_image", .source_node = "input"}};
    config.nodes.push_back(std::move(a));

    return config;
}

// --- Construction ---

TEST(IrisPipeline, ConstructFromValidConfig) {
    auto config = make_mini_config();
    auto result = IrisPipeline::from_config(config);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_TRUE(result->dag().has_node("specular"));
}

TEST(IrisPipeline, ConstructFromInvalidConfig) {
    PipelineConfig config;
    config.pipeline_name = "test";
    config.iris_version = "1.0";
    // Empty — should fail
    auto result = IrisPipeline::from_config(config);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ConfigInvalid);
}

TEST(IrisPipeline, ConstructFromUnknownClassNameFails) {
    PipelineConfig config;
    config.pipeline_name = "test";
    config.iris_version = "1.0";

    NodeConfig node;
    node.name = "bad";
    node.class_name = "iris.NonexistentNode";
    node.inputs = {{.name = "x", .source_node = "input"}};
    config.nodes.push_back(std::move(node));

    auto result = IrisPipeline::from_config(config);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ConfigInvalid);
}

// --- Run ---

TEST(IrisPipeline, RunProducesOutput) {
    auto config = make_mini_config();
    auto pipeline = IrisPipeline::from_config(config);
    ASSERT_TRUE(pipeline.has_value()) << pipeline.error().message;

    IRImage img;
    img.data = cv::Mat::zeros(64, 64, CV_8UC1);

    auto output = pipeline->run(img);
    ASSERT_TRUE(output.has_value());
    // No "encoder" node → no iris_template, but also no error
    EXPECT_FALSE(output->iris_template.has_value());
    EXPECT_FALSE(output->error.has_value());
}

TEST(IrisPipeline, RunWithCancellation) {
    auto config = make_mini_config();
    auto pipeline = IrisPipeline::from_config(config);
    ASSERT_TRUE(pipeline.has_value()) << pipeline.error().message;

    IRImage img;
    img.data = cv::Mat::zeros(64, 64, CV_8UC1);

    CancellationToken token;
    token.cancel();  // Cancel before run

    auto output = pipeline->run(img, &token);
    ASSERT_TRUE(output.has_value());
    // Cancelled → PipelineOutput with error
    ASSERT_TRUE(output->error.has_value());
    EXPECT_EQ(output->error->code, ErrorCode::Cancelled);
}

TEST(IrisPipeline, MultipleRunsAreIndependent) {
    auto config = make_mini_config();
    auto pipeline = IrisPipeline::from_config(config);
    ASSERT_TRUE(pipeline.has_value()) << pipeline.error().message;

    IRImage img1;
    img1.data = cv::Mat::zeros(64, 64, CV_8UC1);

    IRImage img2;
    img2.data = cv::Mat::ones(32, 32, CV_8UC1) * 128;

    auto out1 = pipeline->run(img1);
    auto out2 = pipeline->run(img2);
    ASSERT_TRUE(out1.has_value());
    ASSERT_TRUE(out2.has_value());
    // Both should succeed independently
    EXPECT_FALSE(out1->error.has_value());
    EXPECT_FALSE(out2->error.has_value());
}

}  // namespace iris::test
