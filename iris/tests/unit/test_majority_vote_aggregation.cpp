#include <iris/nodes/majority_vote_aggregation.hpp>

#include <gtest/gtest.h>

using namespace iris;

// --- Helpers ---

/// Create an AlignedTemplates from a vector of (code_fill, mask_fill) pairs.
static AlignedTemplates make_aligned(
    const std::vector<std::pair<uint64_t, uint64_t>>& fills) {
    AlignedTemplates at;
    at.distances = DistanceMatrix(fills.size());
    at.reference_template_id = "img0";
    for (size_t i = 0; i < fills.size(); ++i) {
        IrisTemplateWithId tmpl;
        tmpl.image_id = "img" + std::to_string(i);
        PackedIrisCode ic;
        ic.code_bits.fill(fills[i].first);
        ic.mask_bits.fill(fills[i].second);
        tmpl.iris_codes.push_back(ic);
        PackedIrisCode mc;
        mc.code_bits.fill(fills[i].second);
        mc.mask_bits.fill(fills[i].second);
        tmpl.mask_codes.push_back(mc);
        at.templates.push_back(std::move(tmpl));
    }
    return at;
}

// --- Tests ---

TEST(MajorityVoteAggregation, AllAgree1) {
    MajorityVoteAggregation::Params p;
    p.consistency_threshold = 0.75;
    p.mask_threshold = 0.01;
    MajorityVoteAggregation agg(p);

    // 3 templates, all bits=1, all masked in
    auto input = make_aligned({
        {~uint64_t{0}, ~uint64_t{0}},
        {~uint64_t{0}, ~uint64_t{0}},
        {~uint64_t{0}, ~uint64_t{0}}
    });

    auto result = agg.run(input);
    ASSERT_TRUE(result.has_value());

    // All bits should be 1
    auto [code, mask] = result.value().iris_codes[0].to_mat();
    EXPECT_EQ(cv::countNonZero(code), code.rows * code.cols);
    EXPECT_EQ(cv::countNonZero(mask), mask.rows * mask.cols);

    // Consistency = 1.0 for all bits
    const auto& w = result.value().weights[0];
    for (int r = 0; r < w.rows; ++r) {
        for (int c = 0; c < w.cols; ++c) {
            EXPECT_DOUBLE_EQ(w.at<double>(r, c), 1.0);
        }
    }
}

TEST(MajorityVoteAggregation, AllAgree0) {
    MajorityVoteAggregation agg;
    auto input = make_aligned({
        {0, ~uint64_t{0}},
        {0, ~uint64_t{0}},
        {0, ~uint64_t{0}}
    });

    auto result = agg.run(input);
    ASSERT_TRUE(result.has_value());

    auto [code, mask] = result.value().iris_codes[0].to_mat();
    EXPECT_EQ(cv::countNonZero(code), 0);
    EXPECT_EQ(cv::countNonZero(mask), mask.rows * mask.cols);
}

TEST(MajorityVoteAggregation, MajorityWins) {
    MajorityVoteAggregation::Params p;
    p.consistency_threshold = 0.5;
    MajorityVoteAggregation agg(p);

    // 3 templates: 2 have bit=1, 1 has bit=0 → majority is 1
    // Consistency = 2/3 ≈ 0.667
    auto input = make_aligned({
        {~uint64_t{0}, ~uint64_t{0}},
        {~uint64_t{0}, ~uint64_t{0}},
        {0, ~uint64_t{0}}
    });

    auto result = agg.run(input);
    ASSERT_TRUE(result.has_value());

    auto [code, mask] = result.value().iris_codes[0].to_mat();
    // Majority is 1
    EXPECT_EQ(cv::countNonZero(code), code.rows * code.cols);

    // Consistency = 2/3
    double expected_consistency = 2.0 / 3.0;
    EXPECT_NEAR(result.value().weights[0].at<double>(0, 0),
                expected_consistency, 1e-10);
}

TEST(MajorityVoteAggregation, MaskThreshold) {
    MajorityVoteAggregation::Params p;
    p.mask_threshold = 0.5;  // Need at least 50% of templates with valid mask
    MajorityVoteAggregation agg(p);

    // 4 templates, only 1 has mask=1 → 1/4 = 0.25 < 0.5 → masked out
    auto input = make_aligned({
        {~uint64_t{0}, ~uint64_t{0}},
        {0, 0},
        {0, 0},
        {0, 0}
    });

    auto result = agg.run(input);
    ASSERT_TRUE(result.has_value());

    auto [code, mask] = result.value().iris_codes[0].to_mat();
    EXPECT_EQ(cv::countNonZero(mask), 0);
}

TEST(MajorityVoteAggregation, MaskThresholdLow) {
    MajorityVoteAggregation::Params p;
    p.mask_threshold = 0.01;  // Very low threshold
    MajorityVoteAggregation agg(p);

    // 4 templates, 1 has mask=1 → 1/4 = 0.25 >= 0.01 → kept
    auto input = make_aligned({
        {~uint64_t{0}, ~uint64_t{0}},
        {0, 0},
        {0, 0},
        {0, 0}
    });

    auto result = agg.run(input);
    ASSERT_TRUE(result.has_value());

    auto [code, mask] = result.value().iris_codes[0].to_mat();
    EXPECT_GT(cv::countNonZero(mask), 0);
}

TEST(MajorityVoteAggregation, ConsistencyWeight) {
    MajorityVoteAggregation::Params p;
    p.consistency_threshold = 0.9;  // High threshold
    p.use_inconsistent_bits = false;
    MajorityVoteAggregation agg(p);

    // 3 templates: 2 agree, 1 disagrees → consistency = 2/3 ≈ 0.667 < 0.9
    // With use_inconsistent_bits = false → bit is masked out
    auto input = make_aligned({
        {~uint64_t{0}, ~uint64_t{0}},
        {~uint64_t{0}, ~uint64_t{0}},
        {0, ~uint64_t{0}}
    });

    auto result = agg.run(input);
    ASSERT_TRUE(result.has_value());

    auto [code, mask] = result.value().iris_codes[0].to_mat();
    // All bits should be masked out (consistency too low)
    EXPECT_EQ(cv::countNonZero(mask), 0);
}

TEST(MajorityVoteAggregation, InconsistentWeight) {
    MajorityVoteAggregation::Params p;
    p.consistency_threshold = 0.9;
    p.use_inconsistent_bits = true;
    p.inconsistent_bit_threshold = 0.4;
    MajorityVoteAggregation agg(p);

    // 3 templates: 2 agree, 1 disagrees → consistency = 2/3 < 0.9
    // With use_inconsistent_bits = true → weight = 0.4
    auto input = make_aligned({
        {~uint64_t{0}, ~uint64_t{0}},
        {~uint64_t{0}, ~uint64_t{0}},
        {0, ~uint64_t{0}}
    });

    auto result = agg.run(input);
    ASSERT_TRUE(result.has_value());

    auto [code, mask] = result.value().iris_codes[0].to_mat();
    EXPECT_GT(cv::countNonZero(mask), 0);

    EXPECT_DOUBLE_EQ(result.value().weights[0].at<double>(0, 0), 0.4);
}

TEST(MajorityVoteAggregation, InconsistentDisabled) {
    MajorityVoteAggregation::Params p;
    p.consistency_threshold = 0.9;
    p.use_inconsistent_bits = false;
    MajorityVoteAggregation agg(p);

    // Same setup but inconsistent bits disabled
    auto input = make_aligned({
        {~uint64_t{0}, ~uint64_t{0}},
        {~uint64_t{0}, ~uint64_t{0}},
        {0, ~uint64_t{0}}
    });

    auto result = agg.run(input);
    ASSERT_TRUE(result.has_value());

    // Weight should be 0 for inconsistent bits
    EXPECT_DOUBLE_EQ(result.value().weights[0].at<double>(0, 0), 0.0);
}

TEST(MajorityVoteAggregation, SingleTemplate) {
    MajorityVoteAggregation agg;
    auto input = make_aligned({{0xAAAAAAAAAAAAAAAA, ~uint64_t{0}}});

    auto result = agg.run(input);
    ASSERT_TRUE(result.has_value());

    // Single template: consistency = 1.0 for all bits
    auto [code, mask] = result.value().iris_codes[0].to_mat();
    EXPECT_EQ(cv::countNonZero(mask), mask.rows * mask.cols);

    const auto& w = result.value().weights[0];
    EXPECT_DOUBLE_EQ(w.at<double>(0, 0), 1.0);
}

TEST(MajorityVoteAggregation, MultipleWavelets) {
    MajorityVoteAggregation agg;

    // Manually create templates with 2 wavelet pairs
    AlignedTemplates at;
    at.distances = DistanceMatrix(2);
    at.reference_template_id = "img0";

    for (int t = 0; t < 2; ++t) {
        IrisTemplateWithId tmpl;
        tmpl.image_id = "img" + std::to_string(t);
        for (int w = 0; w < 2; ++w) {
            PackedIrisCode ic;
            ic.code_bits.fill(t == 0 ? ~uint64_t{0} : 0);
            ic.mask_bits.fill(~uint64_t{0});
            tmpl.iris_codes.push_back(ic);
            PackedIrisCode mc;
            mc.code_bits.fill(~uint64_t{0});
            mc.mask_bits.fill(~uint64_t{0});
            tmpl.mask_codes.push_back(mc);
        }
        at.templates.push_back(std::move(tmpl));
    }

    auto result = agg.run(at);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().iris_codes.size(), 2u);
    EXPECT_EQ(result.value().weights.size(), 2u);
}

TEST(MajorityVoteAggregation, EmptyFails) {
    MajorityVoteAggregation agg;
    AlignedTemplates at;

    auto result = agg.run(at);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::TemplateAggregationIncompatible);
}

TEST(MajorityVoteAggregation, NodeParams) {
    std::unordered_map<std::string, std::string> np;
    np["consistency_threshold"] = "0.8";
    np["mask_threshold"] = "0.05";
    np["use_inconsistent_bits"] = "false";
    np["inconsistent_bit_threshold"] = "0.3";

    MajorityVoteAggregation agg(np);
    EXPECT_NEAR(agg.params().consistency_threshold, 0.8, 1e-10);
    EXPECT_NEAR(agg.params().mask_threshold, 0.05, 1e-10);
    EXPECT_FALSE(agg.params().use_inconsistent_bits);
    EXPECT_NEAR(agg.params().inconsistent_bit_threshold, 0.3, 1e-10);
}
