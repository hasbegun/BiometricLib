#include <iris/pipeline/pipeline_dag.hpp>

#include <filesystem>

#include <iris/core/config.hpp>
#include <iris/core/node_registry.hpp>
#include <iris/pipeline/callback_dispatcher.hpp>
#include <iris/pipeline/node_dispatcher.hpp>

#include <gtest/gtest.h>

namespace iris::test {

// --- Helpers ---

/// Build a minimal 3-node linear pipeline for testing.
/// specular → binarization (reusing class) → interpolation
/// Uses only nodes with trivial constructors (no ONNX model loading).
PipelineConfig make_simple_config() {
    PipelineConfig config;
    config.pipeline_name = "test";
    config.iris_version = "1.0";

    // Node A: reads from "input", no deps
    NodeConfig a;
    a.name = "node_a";
    a.class_name = "iris.SpecularReflectionDetection";
    a.inputs = {{.name = "ir_image", .source_node = "input"}};
    config.nodes.push_back(std::move(a));

    // Node B: depends on A
    NodeConfig b;
    b.name = "node_b";
    b.class_name = "iris.MomentOfArea";
    b.inputs = {{.name = "geometries", .source_node = "node_a"}};
    config.nodes.push_back(std::move(b));

    // Node C: depends on B
    NodeConfig c;
    c.name = "node_c";
    c.class_name = "iris.ContourInterpolation";
    c.inputs = {{.name = "polygons", .source_node = "node_b"}};
    config.nodes.push_back(std::move(c));

    return config;
}

// --- Basic DAG building ---

TEST(PipelineDAG, BuildFromSimpleConfig) {
    auto config = make_simple_config();
    auto result = PipelineDAG::build(config);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_TRUE(result->has_node("node_a"));
    EXPECT_TRUE(result->has_node("node_b"));
    EXPECT_TRUE(result->has_node("node_c"));
    EXPECT_FALSE(result->levels().empty());
}

TEST(PipelineDAG, CorrectExecutionLevels) {
    auto config = make_simple_config();
    auto result = PipelineDAG::build(config);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    const auto& levels = result->levels();
    // A has no deps → level 0, B deps on A → level 1, C deps on B → level 2
    ASSERT_EQ(levels.size(), 3u);
    EXPECT_EQ(levels[0].node_names.size(), 1u);
    EXPECT_EQ(levels[0].node_names[0], "node_a");
    EXPECT_EQ(levels[1].node_names.size(), 1u);
    EXPECT_EQ(levels[1].node_names[0], "node_b");
    EXPECT_EQ(levels[2].node_names.size(), 1u);
    EXPECT_EQ(levels[2].node_names[0], "node_c");
}

// --- Parallel nodes in same level ---

TEST(PipelineDAG, ParallelNodesInSameLevel) {
    PipelineConfig config;
    config.pipeline_name = "test";
    config.iris_version = "1.0";

    // Both read from "input" → same level
    NodeConfig a;
    a.name = "node_a";
    a.class_name = "iris.SpecularReflectionDetection";
    a.inputs = {{.name = "ir_image", .source_node = "input"}};
    config.nodes.push_back(std::move(a));

    NodeConfig b;
    b.name = "node_b";
    b.class_name = "iris.MomentOfArea";
    b.inputs = {{.name = "geometries", .source_node = "input"}};
    config.nodes.push_back(std::move(b));

    auto result = PipelineDAG::build(config);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    const auto& levels = result->levels();
    ASSERT_EQ(levels.size(), 1u);
    EXPECT_EQ(levels[0].node_names.size(), 2u);
}

// --- Error: unknown class_name ---

TEST(PipelineDAG, UnknownClassNameFails) {
    PipelineConfig config;
    config.pipeline_name = "test";
    config.iris_version = "1.0";

    NodeConfig node;
    node.name = "bad_node";
    node.class_name = "iris.NonexistentAlgorithm";
    node.inputs = {{.name = "x", .source_node = "input"}};
    config.nodes.push_back(std::move(node));

    auto result = PipelineDAG::build(config);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ConfigInvalid);
}

// --- Error: missing source_node ---

TEST(PipelineDAG, MissingSourceNodeFails) {
    PipelineConfig config;
    config.pipeline_name = "test";
    config.iris_version = "1.0";

    NodeConfig a;
    a.name = "node_a";
    a.class_name = "iris.SpecularReflectionDetection";
    a.inputs = {{.name = "ir_image", .source_node = "input"}};
    config.nodes.push_back(std::move(a));

    NodeConfig bad;
    bad.name = "bad_node";
    bad.class_name = "iris.MomentOfArea";
    bad.inputs = {{.name = "x", .source_node = "nonexistent_node"}};
    config.nodes.push_back(std::move(bad));

    auto result = PipelineDAG::build(config);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ConfigInvalid);
    EXPECT_NE(result.error().message.find("unknown node"), std::string::npos);
}

// --- Error: circular dependency ---

TEST(PipelineDAG, CircularDependencyFails) {
    PipelineConfig config;
    config.pipeline_name = "test";
    config.iris_version = "1.0";

    NodeConfig a;
    a.name = "node_a";
    a.class_name = "iris.ContourInterpolation";
    a.inputs = {{.name = "x", .source_node = "node_b"}};
    config.nodes.push_back(std::move(a));

    NodeConfig b;
    b.name = "node_b";
    b.class_name = "iris.ContourInterpolation";
    b.inputs = {{.name = "x", .source_node = "node_a"}};
    config.nodes.push_back(std::move(b));

    auto result = PipelineDAG::build(config);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ConfigInvalid);
    EXPECT_NE(result.error().message.find("Circular"), std::string::npos);
}

// --- Error: empty pipeline ---

TEST(PipelineDAG, EmptyPipelineFails) {
    PipelineConfig config;
    config.pipeline_name = "test";
    config.iris_version = "1.0";

    auto result = PipelineDAG::build(config);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ConfigInvalid);
}

// --- Single node pipeline ---

TEST(PipelineDAG, SingleNodePipeline) {
    PipelineConfig config;
    config.pipeline_name = "test";
    config.iris_version = "1.0";

    NodeConfig node;
    node.name = "only_node";
    node.class_name = "iris.SpecularReflectionDetection";
    node.inputs = {{.name = "ir_image", .source_node = "input"}};
    config.nodes.push_back(std::move(node));

    auto result = PipelineDAG::build(config);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->levels().size(), 1u);
    EXPECT_EQ(result->levels()[0].node_names.size(), 1u);
}

// --- Duplicate node name fails ---

TEST(PipelineDAG, DuplicateNodeNameFails) {
    PipelineConfig config;
    config.pipeline_name = "test";
    config.iris_version = "1.0";

    NodeConfig node;
    node.name = "dup";
    node.class_name = "iris.SpecularReflectionDetection";
    node.inputs = {{.name = "ir_image", .source_node = "input"}};
    config.nodes.push_back(node);
    config.nodes.push_back(std::move(node));

    auto result = PipelineDAG::build(config);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ConfigInvalid);
    EXPECT_NE(result.error().message.find("Duplicate"), std::string::npos);
}

// --- All pipeline.yaml class_names have dispatch functions ---

TEST(PipelineDAG, AllPipelineYamlClassNamesDispatched) {
    const auto& table = dispatch_table();
    std::vector<std::string> yaml_names = {
        "iris.MultilabelSegmentation.create_from_hugging_face",
        "iris.MultilabelSegmentationBinarization",
        "iris.ContouringAlgorithm",
        "iris.SpecularReflectionDetection",
        "iris.ContourInterpolation",
        "iris.ContourPointNoiseEyeballDistanceFilter",
        "iris.MomentOfArea",
        "iris.BisectorsMethod",
        "iris.Smoothing",
        "iris.FusionExtrapolation",
        "iris.PupilIrisPropertyCalculator",
        "iris.EccentricityOffgazeEstimation",
        "iris.OcclusionCalculator",
        "iris.NoiseMaskUnion",
        "iris.LinearNormalization",
        "iris.SharpnessEstimation",
        "iris.ConvFilterBank",
        "iris.IrisEncoder",
        "iris.IrisBBoxCalculator",
        "iris.nodes.iris_response_refinement.fragile_bits_refinement.FragileBitRefinement",
        "iris.nodes.validators.cross_object_validators.EyeCentersInsideImageValidator",
    };
    for (const auto& name : yaml_names) {
        EXPECT_TRUE(table.contains(name)) << "Missing dispatch for: " << name;
    }
}

// --- Node with callbacks builds correctly ---

TEST(PipelineDAG, NodeWithCallbackBuilds) {
    PipelineConfig config;
    config.pipeline_name = "test";
    config.iris_version = "1.0";

    NodeConfig node;
    node.name = "offgaze";
    node.class_name = "iris.EccentricityOffgazeEstimation";
    node.inputs = {{.name = "geometries", .source_node = "input"}};
    node.callbacks.push_back({
        .class_name = "iris.nodes.validators.object_validators.OffgazeValidator",
        .params = {{"max_allowed_offgaze", "0.8"}},
    });
    config.nodes.push_back(std::move(node));

    auto result = PipelineDAG::build(config);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->node("offgaze").validators.size(), 1u);
}

// --- All pipeline.yaml callback class_names have dispatch functions ---

TEST(PipelineDAG, AllPipelineYamlCallbackNamesDispatched) {
    const auto& table = callback_dispatch_table();
    // Callback validators used in pipeline.yaml (long-form Python paths)
    std::vector<std::string> yaml_callback_names = {
        "iris.nodes.validators.object_validators.Pupil2IrisPropertyValidator",
        "iris.nodes.validators.object_validators.OffgazeValidator",
        "iris.nodes.validators.object_validators.OcclusionValidator",
        "iris.nodes.validators.object_validators.SharpnessValidator",
        "iris.nodes.validators.object_validators.IsMaskTooSmallValidator",
    };
    for (const auto& name : yaml_callback_names) {
        EXPECT_TRUE(table.contains(name)) << "Missing callback dispatch for: " << name;
    }
}

// --- Load real pipeline.yaml and build DAG ---

TEST(PipelineDAG, PipelineYamlLoadsAndBuilds) {
    // Path to the real Python pipeline.yaml (relative to build dir)
    const char* yaml_path = "../../open-iris/src/iris/pipelines/confs/pipeline.yaml";
    if (!std::filesystem::exists(yaml_path)) {
        GTEST_SKIP() << "pipeline.yaml not available (Docker build without open-iris/)";
    }
    auto config_result = load_pipeline_config(yaml_path);
    ASSERT_TRUE(config_result.has_value()) << config_result.error().message;

    auto& config = *config_result;
    EXPECT_FALSE(config.pipeline_name.empty());
    EXPECT_FALSE(config.nodes.empty());

    // Build DAG from the real config
    auto dag_result = PipelineDAG::build(config);
    ASSERT_TRUE(dag_result.has_value()) << dag_result.error().message;

    // Verify all node names parsed and the graph has the expected structure
    EXPECT_GE(dag_result->node_names().size(), 15u)
        << "Expected at least 15 nodes from pipeline.yaml";
}

}  // namespace iris::test
