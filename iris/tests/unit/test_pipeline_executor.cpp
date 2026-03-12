#include <iris/pipeline/pipeline_executor.hpp>

#include <iris/core/cancellation.hpp>
#include <iris/core/config.hpp>
#include <iris/core/thread_pool.hpp>
#include <iris/core/types.hpp>
#include <iris/pipeline/pipeline_context.hpp>
#include <iris/pipeline/pipeline_dag.hpp>

#include <cmath>

#include <gtest/gtest.h>

namespace iris::test {

// --- Helpers ---

/// Build a 2-node pipeline: specular_reflection → moment_orientation
/// Both have trivial constructors, no external deps.
PipelineDAG make_two_node_dag() {
    PipelineConfig config;
    config.pipeline_name = "test";
    config.iris_version = "1.0";

    NodeConfig a;
    a.name = "node_a";
    a.class_name = "iris.SpecularReflectionDetection";
    a.inputs = {{.name = "ir_image", .source_node = "input"}};
    config.nodes.push_back(std::move(a));

    NodeConfig b;
    b.name = "node_b";
    b.class_name = "iris.MomentOfArea";
    b.inputs = {{.name = "geometries", .source_node = "node_a"}};
    config.nodes.push_back(std::move(b));

    auto result = PipelineDAG::build(config);
    return std::move(*result);
}

/// Build a binarization + vectorization pipeline (pair output → indexed input).
PipelineDAG make_pair_chain_dag() {
    PipelineConfig config;
    config.pipeline_name = "test";
    config.iris_version = "1.0";

    NodeConfig bin;
    bin.name = "segmentation_binarization";
    bin.class_name = "iris.MultilabelSegmentationBinarization";
    bin.inputs = {{.name = "segmentation_map", .source_node = "input"}};
    config.nodes.push_back(std::move(bin));

    NodeConfig vec;
    vec.name = "vectorization";
    vec.class_name = "iris.ContouringAlgorithm";
    vec.inputs = {{.name = "geometry_mask",
                   .source_node = "segmentation_binarization",
                   .index = 0}};
    config.nodes.push_back(std::move(vec));

    auto result = PipelineDAG::build(config);
    return std::move(*result);
}

/// Create a 4-channel segmentation map with iris/pupil/eyeball regions.
SegmentationMap make_test_segmap(int rows = 64, int cols = 64) {
    cv::Mat predictions(rows, cols, CV_32FC4, cv::Scalar(0, 0, 0, 0));
    for (int r = 10; r < 54; ++r) {
        for (int c = 10; c < 54; ++c) {
            auto& pixel = predictions.at<cv::Vec4f>(r, c);
            pixel[0] = 0.9f;  // eyeball
            pixel[1] = 0.8f;  // iris
            if (r > 22 && r < 42 && c > 22 && c < 42) {
                pixel[2] = 0.9f;  // pupil
            }
        }
    }
    return SegmentationMap{.predictions = predictions};
}

// --- Single-node execution ---

TEST(PipelineExecutor, SingleNodeExecution) {
    PipelineConfig config;
    config.pipeline_name = "test";
    config.iris_version = "1.0";

    NodeConfig node;
    node.name = "binarization";
    node.class_name = "iris.MultilabelSegmentationBinarization";
    node.inputs = {{.name = "segmentation_map", .source_node = "input"}};
    config.nodes.push_back(std::move(node));

    auto dag = PipelineDAG::build(config);
    ASSERT_TRUE(dag.has_value()) << dag.error().message;

    PipelineContext ctx;
    ctx.set("input", make_test_segmap());

    ThreadPool pool(2);
    PipelineExecutor executor(pool);
    auto result = executor.execute(*dag, ctx);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Pair output should be stored as :0 and :1
    EXPECT_TRUE(ctx.has("binarization:0"));
    EXPECT_TRUE(ctx.has("binarization:1"));
}

// --- Multi-level execution (correct order) ---

TEST(PipelineExecutor, MultiLevelExecution) {
    auto dag = make_pair_chain_dag();

    PipelineContext ctx;
    ctx.set("input", make_test_segmap());

    ThreadPool pool(2);
    PipelineExecutor executor(pool);
    auto result = executor.execute(dag, ctx);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Vectorization output should be stored
    auto polys = ctx.get<GeometryPolygons>("vectorization");
    ASSERT_TRUE(polys.has_value()) << polys.error().message;
    EXPECT_FALSE((*polys)->iris.points.empty());
}

// --- Node failure propagation (Raise strategy) ---

TEST(PipelineExecutor, NodeFailurePropagatesInRaiseMode) {
    PipelineConfig config;
    config.pipeline_name = "test";
    config.iris_version = "1.0";

    NodeConfig node;
    node.name = "node_a";
    node.class_name = "iris.MomentOfArea";
    node.inputs = {{.name = "geometries", .source_node = "input"}};
    config.nodes.push_back(std::move(node));

    auto dag = PipelineDAG::build(config);
    ASSERT_TRUE(dag.has_value());

    PipelineContext ctx;
    // Provide wrong type → will fail at dispatch
    ctx.set("input", 42);

    ThreadPool pool(1);
    PipelineExecutor executor(pool);
    auto result = executor.execute(*dag, ctx);
    EXPECT_FALSE(result.has_value());
}

// --- Node failure in Store mode collects errors ---

TEST(PipelineExecutor, NodeFailureStoresInStoreMode) {
    PipelineConfig config;
    config.pipeline_name = "test";
    config.iris_version = "1.0";

    NodeConfig node;
    node.name = "node_a";
    node.class_name = "iris.MomentOfArea";
    node.inputs = {{.name = "geometries", .source_node = "input"}};
    config.nodes.push_back(std::move(node));

    auto dag = PipelineDAG::build(config);
    ASSERT_TRUE(dag.has_value());

    PipelineContext ctx;
    ctx.set("input", 42);  // wrong type

    ThreadPool pool(1);
    PipelineExecutor executor(pool);
    PipelineExecutor::Options opts;
    opts.strategy = PipelineExecutor::ErrorStrategy::Store;
    auto result = executor.execute(*dag, ctx, opts);
    // Store mode doesn't propagate
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(executor.collected_errors().empty());
}

// --- Cancellation between levels ---

TEST(PipelineExecutor, CancellationBetweenLevels) {
    auto dag = make_two_node_dag();

    PipelineContext ctx;
    IRImage img;
    img.data = cv::Mat::zeros(64, 64, CV_8UC1);
    ctx.set("input", img);

    CancellationToken token;
    token.cancel();  // Cancel before execution starts

    ThreadPool pool(1);
    PipelineExecutor executor(pool);
    PipelineExecutor::Options opts;
    opts.token = &token;
    auto result = executor.execute(dag, ctx, opts);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::Cancelled);
}

// --- Callback validation failure ---

TEST(PipelineExecutor, CallbackValidationFailure) {
    PipelineConfig config;
    config.pipeline_name = "test";
    config.iris_version = "1.0";

    NodeConfig node;
    node.name = "offgaze";
    node.class_name = "iris.EccentricityOffgazeEstimation";
    node.inputs = {{.name = "geometries", .source_node = "input"}};
    // Add a validator with very strict threshold
    node.callbacks.push_back({
        .class_name = "iris.OffgazeValidator",
        .params = {{"max_allowed_offgaze", "0.0001"}},
    });
    config.nodes.push_back(std::move(node));

    auto dag = PipelineDAG::build(config);
    ASSERT_TRUE(dag.has_value()) << dag.error().message;

    // Create polygons that produce a non-zero offgaze
    GeometryPolygons polys;
    for (int i = 0; i < 100; ++i) {
        float angle = static_cast<float>(i) * 2.0f * 3.14159f / 100.0f;
        // Eccentric iris (shifted center) should produce non-zero offgaze
        polys.iris.points.push_back({100.0f + 50.0f * std::cos(angle),
                                     100.0f + 30.0f * std::sin(angle)});
        polys.pupil.points.push_back({110.0f + 15.0f * std::cos(angle),
                                      105.0f + 10.0f * std::sin(angle)});
        polys.eyeball.points.push_back({100.0f + 80.0f * std::cos(angle),
                                        100.0f + 50.0f * std::sin(angle)});
    }

    PipelineContext ctx;
    ctx.set("input", polys);

    ThreadPool pool(1);
    PipelineExecutor executor(pool);
    auto result = executor.execute(*dag, ctx);
    // Either offgaze computation or validation should fail with strict threshold
    EXPECT_FALSE(result.has_value());
}

// --- Parallel execution within a level ---

TEST(PipelineExecutor, ParallelExecutionWithinLevel) {
    PipelineConfig config;
    config.pipeline_name = "test";
    config.iris_version = "1.0";

    // Two independent nodes reading from "input" → same level
    NodeConfig a;
    a.name = "node_a";
    a.class_name = "iris.SpecularReflectionDetection";
    a.inputs = {{.name = "ir_image", .source_node = "input"}};
    config.nodes.push_back(std::move(a));

    NodeConfig b;
    b.name = "node_b";
    b.class_name = "iris.SpecularReflectionDetection";
    b.inputs = {{.name = "ir_image", .source_node = "input"}};
    config.nodes.push_back(std::move(b));

    auto dag = PipelineDAG::build(config);
    ASSERT_TRUE(dag.has_value()) << dag.error().message;
    ASSERT_EQ(dag->levels().size(), 1u);
    ASSERT_EQ(dag->levels()[0].node_names.size(), 2u);

    IRImage img;
    img.data = cv::Mat::zeros(64, 64, CV_8UC1);

    PipelineContext ctx;
    ctx.set("input", img);

    ThreadPool pool(4);
    PipelineExecutor executor(pool);
    auto result = executor.execute(*dag, ctx);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_TRUE(ctx.has("node_a"));
    EXPECT_TRUE(ctx.has("node_b"));
}

}  // namespace iris::test
