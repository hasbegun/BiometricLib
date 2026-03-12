#include <iris/nodes/batch_matcher.hpp>

#include <cmath>

#include <gtest/gtest.h>

using namespace iris;

// --- Helpers ---

static IrisTemplate make_uniform_template(uint64_t fill_code) {
    IrisTemplate tmpl;
    PackedIrisCode ic;
    ic.code_bits.fill(fill_code);
    ic.mask_bits.fill(~uint64_t{0});
    tmpl.iris_codes.push_back(ic);
    PackedIrisCode mc;
    mc.code_bits.fill(~uint64_t{0});
    mc.mask_bits.fill(~uint64_t{0});
    tmpl.mask_codes.push_back(mc);
    return tmpl;
}

// --- 1-vs-N tests ---

TEST(BatchMatcher, OneVsN_Identity) {
    BatchMatcher bm;
    auto probe = make_uniform_template(0);
    std::vector<IrisTemplate> gallery = {
        make_uniform_template(0),
        make_uniform_template(~uint64_t{0}),
        make_uniform_template(0)
    };

    auto result = bm.match_one_vs_n(probe, gallery);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 3u);

    // First and third are identical to probe (HD ≈ 0 after normalisation)
    // Second is opposite
    EXPECT_LT(result.value()[0].distance, result.value()[1].distance);
    EXPECT_LT(result.value()[2].distance, result.value()[1].distance);
}

TEST(BatchMatcher, OneVsN_Empty) {
    BatchMatcher bm;
    auto probe = make_uniform_template(0);
    std::vector<IrisTemplate> gallery;

    auto result = bm.match_one_vs_n(probe, gallery);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST(BatchMatcher, OneVsN_Single) {
    BatchMatcher bm;
    auto probe = make_uniform_template(0);
    std::vector<IrisTemplate> gallery = {make_uniform_template(0)};

    auto result = bm.match_one_vs_n(probe, gallery);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1u);
    // With normalisation (default), identical codes still give a small distance
    EXPECT_GE(result.value()[0].distance, 0.0);
}

// --- N-vs-N tests ---

TEST(BatchMatcher, NvsN_Symmetric) {
    BatchMatcher::Params p;
    p.normalise = false;
    p.rotation_shift = 0;
    BatchMatcher bm(p);

    std::vector<IrisTemplate> templates = {
        make_uniform_template(0),
        make_uniform_template(~uint64_t{0}),
        make_uniform_template(0xAAAAAAAAAAAAAAAA)
    };

    auto result = bm.match_n_vs_n(templates);
    ASSERT_TRUE(result.has_value());

    const auto& dm = result.value();
    EXPECT_EQ(dm.size, 3u);

    // Symmetric
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 3; ++j) {
            EXPECT_DOUBLE_EQ(dm(i, j), dm(j, i));
        }
    }
}

TEST(BatchMatcher, NvsN_DiagonalZero) {
    BatchMatcher::Params p;
    p.normalise = false;
    p.rotation_shift = 0;
    BatchMatcher bm(p);

    std::vector<IrisTemplate> templates = {
        make_uniform_template(0),
        make_uniform_template(~uint64_t{0})
    };

    auto result = bm.match_n_vs_n(templates);
    ASSERT_TRUE(result.has_value());

    // Diagonal should be zero (not computed, initialized to 0)
    EXPECT_DOUBLE_EQ(result.value()(0, 0), 0.0);
    EXPECT_DOUBLE_EQ(result.value()(1, 1), 0.0);
}

TEST(BatchMatcher, NvsN_Single) {
    BatchMatcher bm;
    std::vector<IrisTemplate> templates = {make_uniform_template(0)};

    auto result = bm.match_n_vs_n(templates);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size, 1u);
    EXPECT_DOUBLE_EQ(result.value()(0, 0), 0.0);
}

TEST(BatchMatcher, NvsN_WithRotations) {
    BatchMatcher::Params p;
    p.normalise = false;
    p.rotation_shift = 0;
    BatchMatcher bm(p);

    std::vector<IrisTemplate> templates = {
        make_uniform_template(0),
        make_uniform_template(~uint64_t{0})
    };

    auto result = bm.match_n_vs_n_with_rotations(templates);
    ASSERT_TRUE(result.has_value());

    const auto& pr = result.value();
    EXPECT_DOUBLE_EQ(pr.distances(0, 1), 1.0);
    EXPECT_DOUBLE_EQ(pr.distances(1, 0), 1.0);
    // Rotation should be 0 (no rotation_shift)
    EXPECT_EQ(pr.rotations.at<int32_t>(0, 1), 0);
    // Reverse rotation
    EXPECT_EQ(pr.rotations.at<int32_t>(1, 0), 0);
}

TEST(BatchMatcher, ParallelMatchesSerial) {
    BatchMatcher::Params p;
    p.normalise = false;
    p.rotation_shift = 0;
    BatchMatcher bm(p);

    // Create 5 templates with different patterns
    std::vector<IrisTemplate> templates;
    for (uint64_t v : {uint64_t{0}, ~uint64_t{0}, uint64_t{0xAAAAAAAAAAAAAAAA},
                       uint64_t{0x5555555555555555}, uint64_t{0xFF00FF00FF00FF00}}) {
        templates.push_back(make_uniform_template(v));
    }

    // N-vs-N batch
    auto batch_result = bm.match_n_vs_n(templates);
    ASSERT_TRUE(batch_result.has_value());

    // Compare with serial 1-vs-1 matches
    HammingDistanceMatcher matcher(p);
    for (size_t i = 0; i < templates.size(); ++i) {
        for (size_t j = i + 1; j < templates.size(); ++j) {
            auto serial = matcher.run(templates[i], templates[j]);
            ASSERT_TRUE(serial.has_value());
            EXPECT_DOUBLE_EQ(batch_result.value()(i, j), serial.value().distance)
                << "Mismatch at (" << i << ", " << j << ")";
        }
    }
}
