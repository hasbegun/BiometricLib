#include <iris/pipeline/aggregation_pipeline.hpp>
#include <iris/pipeline/multiframe_pipeline.hpp>

#include <iris/core/cancellation.hpp>
#include <iris/core/types.hpp>

#include <gtest/gtest.h>

namespace iris::test {

// ===========================================================================
// AggregationPipeline
// ===========================================================================

/// Create a simple IrisTemplateWithId with uniform code/mask fill values.
static IrisTemplateWithId make_template(const std::string& id,
                                         uint64_t code_fill,
                                         uint64_t mask_fill) {
    IrisTemplateWithId tmpl;
    tmpl.image_id = id;

    PackedIrisCode ic;
    ic.code_bits.fill(code_fill);
    ic.mask_bits.fill(mask_fill);
    tmpl.iris_codes.push_back(ic);

    PackedIrisCode mc;
    mc.code_bits.fill(mask_fill);
    mc.mask_bits.fill(mask_fill);
    tmpl.mask_codes.push_back(mc);

    return tmpl;
}

TEST(AggregationPipeline, TwoCompatibleTemplates) {
    AggregationPipeline agg;

    std::vector<IrisTemplateWithId> templates;
    templates.push_back(make_template("a", ~uint64_t{0}, ~uint64_t{0}));
    templates.push_back(make_template("b", ~uint64_t{0}, ~uint64_t{0}));

    auto result = agg.run(templates);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_FALSE(result->iris_codes.empty());
    EXPECT_FALSE(result->weights.empty());
}

TEST(AggregationPipeline, TooFewTemplatesFails) {
    AggregationPipeline agg;

    std::vector<IrisTemplateWithId> templates;
    templates.push_back(make_template("a", ~uint64_t{0}, ~uint64_t{0}));

    auto result = agg.run(templates);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ConfigInvalid);
}

TEST(AggregationPipeline, ThreeTemplatesSucceeds) {
    AggregationPipeline agg;

    std::vector<IrisTemplateWithId> templates;
    templates.push_back(make_template("a", ~uint64_t{0}, ~uint64_t{0}));
    templates.push_back(make_template("b", ~uint64_t{0}, ~uint64_t{0}));
    templates.push_back(make_template("c", ~uint64_t{0}, ~uint64_t{0}));

    auto result = agg.run(templates);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->iris_codes.size(), 1u);
}

// ===========================================================================
// MultiframePipeline
// ===========================================================================

/// Build a simple single-node pipeline config for testing.
static PipelineConfig make_mini_config() {
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

TEST(MultiframePipeline, ConstructFromValidConfig) {
    auto config = make_mini_config();
    auto result = MultiframePipeline::from_config(config);
    ASSERT_TRUE(result.has_value()) << result.error().message;
}

TEST(MultiframePipeline, ConstructFromInvalidConfig) {
    PipelineConfig config;
    config.pipeline_name = "test";
    config.iris_version = "1.0";
    // Empty pipeline → should fail
    auto result = MultiframePipeline::from_config(config);
    ASSERT_FALSE(result.has_value());
}

TEST(MultiframePipeline, TooFewImagesFails) {
    auto config = make_mini_config();
    auto pipeline = MultiframePipeline::from_config(config);
    ASSERT_TRUE(pipeline.has_value());

    // Only 1 image but min_successful = 2 by default
    std::vector<IRImage> images;
    IRImage img;
    img.data = cv::Mat::zeros(64, 64, CV_8UC1);
    images.push_back(img);

    auto result = pipeline->run(images);
    EXPECT_FALSE(result.has_value());
}

TEST(MultiframePipeline, CancellationBeforeRun) {
    auto config = make_mini_config();
    auto pipeline = MultiframePipeline::from_config(config);
    ASSERT_TRUE(pipeline.has_value());

    std::vector<IRImage> images;
    for (int i = 0; i < 3; ++i) {
        IRImage img;
        img.data = cv::Mat::zeros(64, 64, CV_8UC1);
        images.push_back(img);
    }

    CancellationToken token;
    token.cancel();

    auto result = pipeline->run(images, &token);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::Cancelled);
}

}  // namespace iris::test
