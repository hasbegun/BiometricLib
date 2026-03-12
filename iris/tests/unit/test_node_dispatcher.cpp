#include <iris/pipeline/node_dispatcher.hpp>

#include <iris/core/node_registry.hpp>
#include <iris/nodes/bisectors_eye_center.hpp>
#include <iris/nodes/contour_extractor.hpp>
#include <iris/nodes/contour_interpolator.hpp>
#include <iris/nodes/encoder.hpp>
#include <iris/nodes/moment_orientation.hpp>
#include <iris/nodes/noise_mask_union.hpp>
#include <iris/nodes/segmentation_binarizer.hpp>
#include <iris/nodes/specular_reflection_detector.hpp>

#include <opencv2/imgproc.hpp>

#include <gtest/gtest.h>

namespace iris::test {

// --- Helper: make a minimal NodeConfig ---

NodeConfig make_config(
    const std::string& name,
    const std::string& class_name,
    std::vector<NodeConfig::Input> inputs) {
    NodeConfig cfg;
    cfg.name = name;
    cfg.class_name = class_name;
    cfg.inputs = std::move(inputs);
    return cfg;
}

// --- dispatch_table coverage ---

TEST(NodeDispatcher, DispatchTableHasAllPipelineNodes) {
    const auto& table = dispatch_table();
    // Core pipeline nodes
    EXPECT_TRUE(table.contains("iris.MultilabelSegmentation.create_from_hugging_face"));
    EXPECT_TRUE(table.contains("iris.MultilabelSegmentationBinarization"));
    EXPECT_TRUE(table.contains("iris.ContouringAlgorithm"));
    EXPECT_TRUE(table.contains("iris.SpecularReflectionDetection"));
    EXPECT_TRUE(table.contains("iris.ContourInterpolation"));
    EXPECT_TRUE(table.contains("iris.ContourPointNoiseEyeballDistanceFilter"));
    EXPECT_TRUE(table.contains("iris.MomentOfArea"));
    EXPECT_TRUE(table.contains("iris.BisectorsMethod"));
    EXPECT_TRUE(table.contains("iris.Smoothing"));
    EXPECT_TRUE(table.contains("iris.FusionExtrapolation"));
    EXPECT_TRUE(table.contains("iris.PupilIrisPropertyCalculator"));
    EXPECT_TRUE(table.contains("iris.EccentricityOffgazeEstimation"));
    EXPECT_TRUE(table.contains("iris.OcclusionCalculator"));
    EXPECT_TRUE(table.contains("iris.NoiseMaskUnion"));
    EXPECT_TRUE(table.contains("iris.LinearNormalization"));
    EXPECT_TRUE(table.contains("iris.SharpnessEstimation"));
    EXPECT_TRUE(table.contains("iris.ConvFilterBank"));
    EXPECT_TRUE(table.contains("iris.FragileBitRefinement"));
    EXPECT_TRUE(table.contains("iris.IrisEncoder"));
    EXPECT_TRUE(table.contains("iris.IrisBBoxCalculator"));
    // Long-form names
    EXPECT_TRUE(table.contains(
        "iris.nodes.iris_response_refinement.fragile_bits_refinement.FragileBitRefinement"));
    EXPECT_TRUE(table.contains(
        "iris.nodes.validators.cross_object_validators.EyeCentersInsideImageValidator"));
}

TEST(NodeDispatcher, LookupUnknownClassNameFails) {
    auto result = lookup_dispatch("iris.NonexistentNode");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ConfigInvalid);
}

TEST(NodeDispatcher, LookupKnownClassNameSucceeds) {
    auto result = lookup_dispatch("iris.IrisEncoder");
    EXPECT_TRUE(result.has_value());
}

// --- Dispatch single-input node (MomentOrientation) ---

TEST(NodeDispatcher, Dispatch1InputNode) {
    // MomentOrientation: GeometryPolygons → EyeOrientation
    // Create a minimal GeometryPolygons with a simple iris contour
    GeometryPolygons polys;
    // Need at least an iris contour to compute orientation
    for (int i = 0; i < 100; ++i) {
        float angle = static_cast<float>(i) * 2.0f * 3.14159f / 100.0f;
        polys.iris.points.push_back({100.0f + 50.0f * std::cos(angle),
                                     100.0f + 30.0f * std::sin(angle)});
        polys.pupil.points.push_back({100.0f + 20.0f * std::cos(angle),
                                      100.0f + 12.0f * std::sin(angle)});
        polys.eyeball.points.push_back({100.0f + 80.0f * std::cos(angle),
                                        100.0f + 50.0f * std::sin(angle)});
    }

    PipelineContext ctx;
    ctx.set("distance_filter", polys);

    auto config = make_config("eye_orientation", "iris.MomentOfArea",
                              {{.name = "geometries", .source_node = "distance_filter"}});
    MomentOrientation algo(NodeParams{});
    std::any algo_any = algo;

    auto disp = lookup_dispatch("iris.MomentOfArea");
    ASSERT_TRUE(disp.has_value());
    auto result = (*disp)(algo_any, config, ctx);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Output should be stored in context
    auto out = ctx.get<EyeOrientation>("eye_orientation");
    ASSERT_TRUE(out.has_value());
}

// --- Dispatch pair-producing node (SegmentationBinarizer) ---

TEST(NodeDispatcher, DispatchPairOutput) {
    // SegmentationBinarizer: SegmentationMap → pair<GeometryMask, NoiseMask>
    // Create a 4-channel segmentation map (eyeball, iris, pupil, eyelashes)
    int rows = 64, cols = 64;
    cv::Mat predictions(rows, cols, CV_32FC4, cv::Scalar(0, 0, 0, 0));
    // Set a small central region to iris + pupil
    for (int r = 20; r < 44; ++r) {
        for (int c = 20; c < 44; ++c) {
            auto& pixel = predictions.at<cv::Vec4f>(r, c);
            pixel[0] = 0.9f;  // eyeball
            pixel[1] = 0.8f;  // iris
            if (r > 26 && r < 38 && c > 26 && c < 38) {
                pixel[2] = 0.9f;  // pupil
            }
        }
    }
    SegmentationMap segmap{.predictions = predictions};

    PipelineContext ctx;
    ctx.set("segmentation", segmap);

    auto config = make_config("segmentation_binarization",
                              "iris.MultilabelSegmentationBinarization",
                              {{.name = "segmentation_map", .source_node = "segmentation"}});
    SegmentationBinarizer algo(NodeParams{});
    std::any algo_any = algo;

    auto disp = lookup_dispatch("iris.MultilabelSegmentationBinarization");
    ASSERT_TRUE(disp.has_value());
    auto result = (*disp)(algo_any, config, ctx);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Both indexed outputs stored
    auto geom = ctx.get<GeometryMask>("segmentation_binarization:0");
    ASSERT_TRUE(geom.has_value());
    auto noise = ctx.get<NoiseMask>("segmentation_binarization:1");
    ASSERT_TRUE(noise.has_value());
}

// --- Dispatch indexed input (ContourExtractor reads from pair :0) ---

TEST(NodeDispatcher, DispatchIndexedInput) {
    // ContourExtractor reads GeometryMask — stored as "segmentation_binarization:0"
    int rows = 64, cols = 64;
    GeometryMask geom;
    geom.pupil = cv::Mat::zeros(rows, cols, CV_8UC1);
    geom.iris = cv::Mat::zeros(rows, cols, CV_8UC1);
    geom.eyeball = cv::Mat::zeros(rows, cols, CV_8UC1);
    // Draw circles for contour extraction
    cv::circle(geom.pupil, cv::Point(32, 32), 10, cv::Scalar(255), -1);
    cv::circle(geom.iris, cv::Point(32, 32), 20, cv::Scalar(255), -1);
    cv::circle(geom.eyeball, cv::Point(32, 32), 30, cv::Scalar(255), -1);

    PipelineContext ctx;
    ctx.set("segmentation_binarization:0", geom);

    auto config = make_config("vectorization", "iris.ContouringAlgorithm",
                              {{.name = "geometry_mask",
                                .source_node = "segmentation_binarization",
                                .index = 0}});
    ContourExtractor algo(NodeParams{});
    std::any algo_any = algo;

    auto disp = lookup_dispatch("iris.ContouringAlgorithm");
    ASSERT_TRUE(disp.has_value());
    auto result = (*disp)(algo_any, config, ctx);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    auto out = ctx.get<GeometryPolygons>("vectorization");
    ASSERT_TRUE(out.has_value());
    EXPECT_FALSE((*out)->iris.points.empty());
}

// --- Missing input error ---

TEST(NodeDispatcher, MissingInputReturnsError) {
    PipelineContext ctx;
    // No "distance_filter" in context
    auto config = make_config("eye_orientation", "iris.MomentOfArea",
                              {{.name = "geometries", .source_node = "distance_filter"}});
    MomentOrientation algo(NodeParams{});
    std::any algo_any = algo;

    auto disp = lookup_dispatch("iris.MomentOfArea");
    ASSERT_TRUE(disp.has_value());
    auto result = (*disp)(algo_any, config, ctx);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ConfigInvalid);
}

// --- Multi-source dispatch (NoiseMaskUnion) ---

TEST(NodeDispatcher, DispatchMultiSourceNoiseMaskUnion) {
    int rows = 32, cols = 32;
    NoiseMask mask1{.mask = cv::Mat::zeros(rows, cols, CV_8UC1)};
    NoiseMask mask2{.mask = cv::Mat::zeros(rows, cols, CV_8UC1)};
    // Set different regions
    mask1.mask(cv::Rect(0, 0, 16, 32)).setTo(255);
    mask2.mask(cv::Rect(16, 0, 16, 32)).setTo(255);

    PipelineContext ctx;
    ctx.set("segmentation_binarization:1", mask1);
    ctx.set("specular_reflection_detection", mask2);

    // Multi-source format stored by config parser
    auto config = make_config(
        "noise_masks_aggregation", "iris.NoiseMaskUnion",
        {{.name = "elements",
          .source_node = "segmentation_binarization:1,specular_reflection_detection"}});

    NoiseMaskUnion algo(NodeParams{});
    std::any algo_any = algo;

    auto disp = lookup_dispatch("iris.NoiseMaskUnion");
    ASSERT_TRUE(disp.has_value());
    auto result = (*disp)(algo_any, config, ctx);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    auto out = ctx.get<NoiseMask>("noise_masks_aggregation");
    ASSERT_TRUE(out.has_value());
    // Union should have non-zero pixels from both masks
    EXPECT_GT(cv::countNonZero((*out)->mask), 0);
}

// --- read_input helper ---

TEST(NodeDispatcher, ReadInputWithIndex) {
    PipelineContext ctx;
    ctx.set("node:0", 42);
    ctx.set("node:1", 99);

    NodeConfig::Input inp{.name = "x", .source_node = "node", .index = 0};
    auto r = read_input<int>(ctx, inp);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(**r, 42);

    inp.index = 1;
    r = read_input<int>(ctx, inp);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(**r, 99);
}

TEST(NodeDispatcher, ReadInputWithoutIndex) {
    PipelineContext ctx;
    ctx.set("some_node", std::string("hello"));

    NodeConfig::Input inp{.name = "x", .source_node = "some_node"};
    auto r = read_input<std::string>(ctx, inp);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(**r, "hello");
}

}  // namespace iris::test
