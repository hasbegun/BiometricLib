#include <iris/pipeline/iris_pipeline.hpp>

#include <iris/core/cancellation.hpp>
#include <iris/core/config.hpp>
#include <iris/core/thread_pool.hpp>
#include <iris/core/types.hpp>
#include <iris/pipeline/pipeline_context.hpp>
#include <iris/pipeline/pipeline_dag.hpp>
#include <iris/pipeline/pipeline_executor.hpp>

#include <cmath>

#include <gtest/gtest.h>

namespace iris::test {

// --- Helpers ---

/// Create a 4-channel segmentation map with realistic iris/pupil/eyeball.
static SegmentationMap make_segmap(int rows = 64, int cols = 64) {
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

/// Build a post-segmentation pipeline config (binarization → vectorization).
/// This avoids loading ONNX models.
static PipelineConfig make_post_seg_config() {
    PipelineConfig config;
    config.pipeline_name = "integration_test";
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

    NodeConfig interp;
    interp.name = "interpolation";
    interp.class_name = "iris.ContourInterpolation";
    interp.inputs = {{.name = "polygons", .source_node = "vectorization"}};
    config.nodes.push_back(std::move(interp));

    return config;
}

// --- Integration: binarization → vectorization → interpolation ---

TEST(PipelineIntegration, PostSegmentationChain) {
    auto config = make_post_seg_config();
    auto dag = PipelineDAG::build(config);
    ASSERT_TRUE(dag.has_value()) << dag.error().message;

    PipelineContext ctx;
    ctx.set("input", make_segmap());

    ThreadPool pool(2);
    PipelineExecutor executor(pool);
    auto result = executor.execute(*dag, ctx);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Verify chain produced output at each stage
    EXPECT_TRUE(ctx.has("segmentation_binarization:0"));
    EXPECT_TRUE(ctx.has("segmentation_binarization:1"));
    EXPECT_TRUE(ctx.has("vectorization"));
    EXPECT_TRUE(ctx.has("interpolation"));

    auto polys = ctx.get<GeometryPolygons>("interpolation");
    ASSERT_TRUE(polys.has_value()) << polys.error().message;
    EXPECT_FALSE((*polys)->iris.points.empty());
    EXPECT_FALSE((*polys)->pupil.points.empty());
}

// --- IrisPipeline facade with post-seg config ---

TEST(PipelineIntegration, IrisPipelineFacadePostSeg) {
    auto config = make_post_seg_config();
    auto pipeline = IrisPipeline::from_config(config);
    ASSERT_TRUE(pipeline.has_value()) << pipeline.error().message;

    // IrisPipeline expects IRImage as input, but our config starts from
    // segmentation_map. We test from_config succeeds and DAG is valid.
    EXPECT_TRUE(pipeline->dag().has_node("segmentation_binarization"));
    EXPECT_TRUE(pipeline->dag().has_node("vectorization"));
    EXPECT_TRUE(pipeline->dag().has_node("interpolation"));
    EXPECT_EQ(pipeline->dag().levels().size(), 3u);
}

// --- Sequential vs parallel parity ---

TEST(PipelineIntegration, SequentialVsParallelParity) {
    auto config = make_post_seg_config();
    auto dag = PipelineDAG::build(config);
    ASSERT_TRUE(dag.has_value());

    auto segmap = make_segmap();

    // Run with 1 thread (sequential)
    PipelineContext ctx1;
    ctx1.set("input", segmap);
    ThreadPool pool1(1);
    PipelineExecutor exec1(pool1);
    auto r1 = exec1.execute(*dag, ctx1);
    ASSERT_TRUE(r1.has_value()) << r1.error().message;

    // Run with 4 threads (parallel)
    PipelineContext ctx2;
    ctx2.set("input", segmap);
    ThreadPool pool2(4);
    PipelineExecutor exec2(pool2);
    auto r2 = exec2.execute(*dag, ctx2);
    ASSERT_TRUE(r2.has_value()) << r2.error().message;

    // Both should produce identical outputs
    auto p1 = ctx1.get<GeometryPolygons>("interpolation");
    auto p2 = ctx2.get<GeometryPolygons>("interpolation");
    ASSERT_TRUE(p1.has_value());
    ASSERT_TRUE(p2.has_value());
    EXPECT_EQ((*p1)->iris.points.size(), (*p2)->iris.points.size());
    EXPECT_EQ((*p1)->pupil.points.size(), (*p2)->pupil.points.size());
}

// --- Config validation errors ---

TEST(PipelineIntegration, BadClassNameFails) {
    PipelineConfig config;
    config.pipeline_name = "test";
    config.iris_version = "1.0";

    NodeConfig node;
    node.name = "bad";
    node.class_name = "iris.DoesNotExist";
    node.inputs = {{.name = "x", .source_node = "input"}};
    config.nodes.push_back(std::move(node));

    auto result = IrisPipeline::from_config(config);
    EXPECT_FALSE(result.has_value());
}

TEST(PipelineIntegration, CircularDependencyFails) {
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

    auto result = IrisPipeline::from_config(config);
    EXPECT_FALSE(result.has_value());
}

// --- Cancellation mid-pipeline ---

TEST(PipelineIntegration, CancellationMidPipeline) {
    auto config = make_post_seg_config();
    auto dag = PipelineDAG::build(config);
    ASSERT_TRUE(dag.has_value());

    PipelineContext ctx;
    ctx.set("input", make_segmap());

    CancellationToken token;
    token.cancel();

    ThreadPool pool(1);
    PipelineExecutor executor(pool);
    PipelineExecutor::Options opts;
    opts.token = &token;
    auto result = executor.execute(*dag, ctx, opts);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::Cancelled);
}

// --- Missing source_node ---

TEST(PipelineIntegration, MissingSourceNodeFails) {
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
    b.inputs = {{.name = "x", .source_node = "nonexistent_node"}};
    config.nodes.push_back(std::move(b));

    auto result = IrisPipeline::from_config(config);
    EXPECT_FALSE(result.has_value());
}

// --- Cancellation under concurrent load ---

TEST(PipelineIntegration, CancellationMidPipelineConcurrent) {
    auto config = make_post_seg_config();
    auto dag = PipelineDAG::build(config);
    ASSERT_TRUE(dag.has_value());

    PipelineContext ctx;
    ctx.set("input", make_segmap());

    // Cancel immediately — verify no crash/hang with 4 threads
    CancellationToken token;
    token.cancel();

    ThreadPool pool(4);
    PipelineExecutor executor(pool);
    PipelineExecutor::Options opts;
    opts.token = &token;
    auto result = executor.execute(*dag, ctx, opts);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::Cancelled);
}

// --- Repeated sequential runs produce identical results ---

TEST(PipelineIntegration, RepeatedRunsDeterministic) {
    auto config = make_post_seg_config();
    auto dag = PipelineDAG::build(config);
    ASSERT_TRUE(dag.has_value());

    auto segmap = make_segmap();

    // Run 3 times with parallel pool, collect results
    std::vector<size_t> iris_counts;
    std::vector<size_t> pupil_counts;
    for (int i = 0; i < 3; ++i) {
        PipelineContext ctx;
        ctx.set("input", segmap);
        ThreadPool pool(4);
        PipelineExecutor executor(pool);
        auto result = executor.execute(*dag, ctx);
        ASSERT_TRUE(result.has_value()) << result.error().message;

        auto polys = ctx.get<GeometryPolygons>("interpolation");
        ASSERT_TRUE(polys.has_value());
        iris_counts.push_back((*polys)->iris.points.size());
        pupil_counts.push_back((*polys)->pupil.points.size());
    }

    // All runs should produce identical point counts
    for (size_t i = 1; i < iris_counts.size(); ++i) {
        EXPECT_EQ(iris_counts[i], iris_counts[0]) << "Run " << i << " differs";
        EXPECT_EQ(pupil_counts[i], pupil_counts[0]) << "Run " << i << " differs";
    }
}

}  // namespace iris::test
