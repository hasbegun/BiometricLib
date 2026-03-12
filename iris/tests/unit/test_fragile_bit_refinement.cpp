#include <iris/nodes/fragile_bit_refinement.hpp>

#include <cmath>

#include <gtest/gtest.h>

using namespace iris;

// --- Helpers ---

/// Create a single-response IrisFilterResponse with given values.
static IrisFilterResponse make_response(int rows, int cols,
                                         cv::Vec2d data_fill,
                                         cv::Vec2d mask_fill) {
    IrisFilterResponse resp;
    resp.responses.push_back({
        cv::Mat(rows, cols, CV_64FC2, cv::Scalar(data_fill[0], data_fill[1])),
        cv::Mat(rows, cols, CV_64FC2, cv::Scalar(mask_fill[0], mask_fill[1]))
    });
    return resp;
}

// --- Polar mode tests ---

TEST(FragileBitRefinement, PolarRadiusBelowMin) {
    // Radius below threshold[0] → mask zeroed
    FragileBitRefinement::Params p;
    p.fragile_type = "polar";
    p.value_threshold = {0.1, 1.0, 1.0};
    p.mask_is_duplicated = false;
    FragileBitRefinement fbr(p);

    // data = (0.01, 0.01), radius = ~0.014 < 0.1
    auto input = make_response(4, 8, {0.01, 0.01}, {1.0, 1.0});
    auto result = fbr.run(input);
    ASSERT_TRUE(result.has_value());

    const auto& mask = result.value().responses[0].mask;
    for (int r = 0; r < mask.rows; ++r) {
        const auto* row = mask.ptr<cv::Vec2d>(r);
        for (int c = 0; c < mask.cols; ++c) {
            EXPECT_DOUBLE_EQ(row[c][0], 0.0);
            EXPECT_DOUBLE_EQ(row[c][1], 0.0);
        }
    }
}

TEST(FragileBitRefinement, PolarRadiusAboveMax) {
    // Radius above threshold[1] → mask zeroed
    FragileBitRefinement::Params p;
    p.fragile_type = "polar";
    p.value_threshold = {0.0, 0.1, 1.0};
    p.mask_is_duplicated = false;
    FragileBitRefinement fbr(p);

    // data = (0.5, 0.5), radius = ~0.707 > 0.1
    auto input = make_response(4, 8, {0.5, 0.5}, {1.0, 1.0});
    auto result = fbr.run(input);
    ASSERT_TRUE(result.has_value());

    const auto& mask = result.value().responses[0].mask;
    for (int r = 0; r < mask.rows; ++r) {
        const auto* row = mask.ptr<cv::Vec2d>(r);
        for (int c = 0; c < mask.cols; ++c) {
            EXPECT_DOUBLE_EQ(row[c][0], 0.0);
            EXPECT_DOUBLE_EQ(row[c][1], 0.0);
        }
    }
}

TEST(FragileBitRefinement, PolarRadiusInRange) {
    // Radius in range, angle check with large threshold → mask preserved
    FragileBitRefinement::Params p;
    p.fragile_type = "polar";
    p.value_threshold = {0.0, 10.0, 1.5};  // cos(1.5) ≈ 0.07, very permissive
    p.mask_is_duplicated = false;
    FragileBitRefinement fbr(p);

    // data = (0.0, 0.3) → radius=0.3, angle=pi/2, cos(pi/2)≈0, sin(pi/2)≈1
    // cos_thresh = |cos(1.5)| ≈ 0.07
    // cos_ok: |cos(pi/2)| ≈ 0 <= 0.07 → true → imag mask preserved
    // sin_ok: |sin(pi/2)| ≈ 1 > 0.07 → false → real mask zeroed
    auto input = make_response(2, 4, {0.0, 0.3}, {1.0, 1.0});
    auto result = fbr.run(input);
    ASSERT_TRUE(result.has_value());

    const auto& mask = result.value().responses[0].mask;
    const auto* row = mask.ptr<cv::Vec2d>(0);
    // Real mask zeroed (sin_ok failed), imag mask preserved (cos_ok passed)
    EXPECT_DOUBLE_EQ(row[0][0], 0.0);
    EXPECT_DOUBLE_EQ(row[0][1], 1.0);
}

TEST(FragileBitRefinement, PolarMaskDuplicated) {
    FragileBitRefinement::Params p;
    p.fragile_type = "polar";
    p.value_threshold = {0.0, 10.0, 1.5};
    p.mask_is_duplicated = true;
    FragileBitRefinement fbr(p);

    auto input = make_response(2, 4, {0.0, 0.3}, {0.8, 0.9});
    auto result = fbr.run(input);
    ASSERT_TRUE(result.has_value());

    // When duplicated, both channels get the same value
    const auto& mask = result.value().responses[0].mask;
    for (int r = 0; r < mask.rows; ++r) {
        const auto* row = mask.ptr<cv::Vec2d>(r);
        for (int c = 0; c < mask.cols; ++c) {
            EXPECT_DOUBLE_EQ(row[c][0], row[c][1]);
        }
    }
}

// --- Cartesian mode tests ---

TEST(FragileBitRefinement, CartesianBelowMinThreshold) {
    FragileBitRefinement::Params p;
    p.fragile_type = "cartesian";
    p.value_threshold = {0.1, 1.0, 1.0};
    p.mask_is_duplicated = false;
    FragileBitRefinement fbr(p);

    // |real|=0.05 < 0.1, |imag|=0.05 < 0.1 → both masked
    auto input = make_response(2, 4, {0.05, 0.05}, {1.0, 1.0});
    auto result = fbr.run(input);
    ASSERT_TRUE(result.has_value());

    const auto& mask = result.value().responses[0].mask;
    const auto* row = mask.ptr<cv::Vec2d>(0);
    EXPECT_DOUBLE_EQ(row[0][0], 0.0);
    EXPECT_DOUBLE_EQ(row[0][1], 0.0);
}

TEST(FragileBitRefinement, CartesianInRange) {
    FragileBitRefinement::Params p;
    p.fragile_type = "cartesian";
    p.value_threshold = {0.01, 1.0, 1.0};
    p.mask_is_duplicated = false;
    FragileBitRefinement fbr(p);

    // |real|=0.5, |imag|=0.5, both in [0.01, 1.0] → both preserved
    auto input = make_response(2, 4, {0.5, 0.5}, {0.95, 0.95});
    auto result = fbr.run(input);
    ASSERT_TRUE(result.has_value());

    const auto& mask = result.value().responses[0].mask;
    const auto* row = mask.ptr<cv::Vec2d>(0);
    EXPECT_DOUBLE_EQ(row[0][0], 0.95);
    EXPECT_DOUBLE_EQ(row[0][1], 0.95);
}

TEST(FragileBitRefinement, CartesianMaskDuplicated) {
    FragileBitRefinement::Params p;
    p.fragile_type = "cartesian";
    p.value_threshold = {0.01, 1.0, 1.0};
    p.mask_is_duplicated = true;
    FragileBitRefinement fbr(p);

    auto input = make_response(2, 4, {0.5, 0.5}, {0.8, 0.9});
    auto result = fbr.run(input);
    ASSERT_TRUE(result.has_value());

    // Duplicated: both channels get the same value (from imag channel mask)
    const auto& mask = result.value().responses[0].mask;
    for (int r = 0; r < mask.rows; ++r) {
        const auto* row = mask.ptr<cv::Vec2d>(r);
        for (int c = 0; c < mask.cols; ++c) {
            EXPECT_DOUBLE_EQ(row[c][0], row[c][1]);
        }
    }
}

TEST(FragileBitRefinement, DataPassedThrough) {
    FragileBitRefinement fbr;
    auto input = make_response(4, 8, {0.5, -0.3}, {1.0, 1.0});
    auto result = fbr.run(input);
    ASSERT_TRUE(result.has_value());

    // Data should be unchanged
    cv::Mat diff;
    cv::absdiff(result.value().responses[0].data, input.responses[0].data, diff);
    EXPECT_EQ(cv::countNonZero(diff.reshape(1)), 0);
}

TEST(FragileBitRefinement, NodeParamsConstruction) {
    std::unordered_map<std::string, std::string> np;
    np["fragile_type"] = "cartesian";
    np["mask_is_duplicated"] = "true";
    np["value_threshold"] = "0.001,0.5,0.2";

    FragileBitRefinement fbr(np);
    EXPECT_EQ(fbr.params().fragile_type, "cartesian");
    EXPECT_TRUE(fbr.params().mask_is_duplicated);
    EXPECT_NEAR(fbr.params().value_threshold[0], 0.001, 1e-10);
    EXPECT_NEAR(fbr.params().value_threshold[1], 0.5, 1e-10);
    EXPECT_NEAR(fbr.params().value_threshold[2], 0.2, 1e-10);
}
