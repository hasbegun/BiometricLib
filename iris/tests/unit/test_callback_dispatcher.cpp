#include <iris/pipeline/callback_dispatcher.hpp>

#include <iris/core/node_registry.hpp>
#include <iris/nodes/object_validators.hpp>

#include <gtest/gtest.h>

namespace iris::test {

// --- Long-form registry aliases resolve ---

TEST(CallbackDispatcher, LongFormValidatorNamesRegistered) {
    const auto& reg = NodeRegistry::instance();
    // Object validators
    EXPECT_TRUE(reg.has(
        "iris.nodes.validators.object_validators.Pupil2IrisPropertyValidator"));
    EXPECT_TRUE(reg.has(
        "iris.nodes.validators.object_validators.OffgazeValidator"));
    EXPECT_TRUE(reg.has(
        "iris.nodes.validators.object_validators.OcclusionValidator"));
    EXPECT_TRUE(reg.has(
        "iris.nodes.validators.object_validators.SharpnessValidator"));
    EXPECT_TRUE(reg.has(
        "iris.nodes.validators.object_validators.IsMaskTooSmallValidator"));
    // Cross-object validators
    EXPECT_TRUE(reg.has(
        "iris.nodes.validators.cross_object_validators.EyeCentersInsideImageValidator"));
    EXPECT_TRUE(reg.has(
        "iris.nodes.validators.cross_object_validators.ExtrapolatedPolygonsInsideImageValidator"));
    // FragileBitRefinement
    EXPECT_TRUE(reg.has(
        "iris.nodes.iris_response_refinement.fragile_bits_refinement.FragileBitRefinement"));
}

// --- Callback dispatch table coverage ---

TEST(CallbackDispatcher, TableHasAllPipelineValidators) {
    const auto& table = callback_dispatch_table();
    // Short forms
    EXPECT_TRUE(table.contains("iris.Pupil2IrisPropertyValidator"));
    EXPECT_TRUE(table.contains("iris.OffgazeValidator"));
    EXPECT_TRUE(table.contains("iris.OcclusionValidator"));
    EXPECT_TRUE(table.contains("iris.SharpnessValidator"));
    EXPECT_TRUE(table.contains("iris.IsMaskTooSmallValidator"));
    // Long forms
    EXPECT_TRUE(table.contains(
        "iris.nodes.validators.object_validators.Pupil2IrisPropertyValidator"));
    EXPECT_TRUE(table.contains(
        "iris.nodes.validators.object_validators.OffgazeValidator"));
}

TEST(CallbackDispatcher, LookupUnknownFails) {
    auto result = lookup_callback_dispatch("iris.NonexistentValidator");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ConfigInvalid);
}

// --- Callback passes ---

TEST(CallbackDispatcher, OffgazeValidatorPasses) {
    OffgazeValidator validator(NodeParams{});
    // Default max_allowed_offgaze = 0.8, so 0.1 should pass
    Offgaze offgaze{.score = 0.1};

    auto disp = lookup_callback_dispatch("iris.OffgazeValidator");
    ASSERT_TRUE(disp.has_value());
    auto result = (*disp)(std::any(validator), std::any(offgaze));
    EXPECT_TRUE(result.has_value());
}

// --- Callback fails ---

TEST(CallbackDispatcher, OffgazeValidatorFails) {
    NodeParams params;
    params["max_allowed_offgaze"] = "0.3";
    OffgazeValidator validator(params);
    Offgaze offgaze{.score = 0.5};  // exceeds 0.3

    auto disp = lookup_callback_dispatch("iris.OffgazeValidator");
    ASSERT_TRUE(disp.has_value());
    auto result = (*disp)(std::any(validator), std::any(offgaze));
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ValidationFailed);
}

// --- Wrong output type error ---

TEST(CallbackDispatcher, WrongOutputTypeReturnsError) {
    OffgazeValidator validator(NodeParams{});
    // Pass wrong type — a string instead of Offgaze
    auto disp = lookup_callback_dispatch("iris.OffgazeValidator");
    ASSERT_TRUE(disp.has_value());
    auto result = (*disp)(std::any(validator), std::any(std::string("wrong")));
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ConfigInvalid);
}

}  // namespace iris::test
