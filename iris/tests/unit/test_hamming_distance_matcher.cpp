#include <iris/nodes/hamming_distance_matcher.hpp>

#include <cmath>

#include <gtest/gtest.h>

using namespace iris;

// --- Helpers ---

/// Create a template with one code pair, all bits set to `val` (0 or 1),
/// with full mask (all 1s).
static IrisTemplate make_uniform_template(uint64_t fill_code, uint64_t fill_mask = ~uint64_t{0}) {
    IrisTemplate tmpl;
    PackedIrisCode ic;
    ic.code_bits.fill(fill_code);
    ic.mask_bits.fill(fill_mask);
    tmpl.iris_codes.push_back(ic);
    // mask_codes mirrors iris_codes but code_bits = mask, mask_bits = mask
    PackedIrisCode mc;
    mc.code_bits.fill(fill_mask);
    mc.mask_bits.fill(fill_mask);
    tmpl.mask_codes.push_back(mc);
    return tmpl;
}

/// Create a template with two code pairs.
static IrisTemplate make_two_wavelet_template(uint64_t fill0, uint64_t fill1) {
    IrisTemplate tmpl;
    for (uint64_t fill : {fill0, fill1}) {
        PackedIrisCode ic;
        ic.code_bits.fill(fill);
        ic.mask_bits.fill(~uint64_t{0});
        tmpl.iris_codes.push_back(ic);

        PackedIrisCode mc;
        mc.code_bits.fill(~uint64_t{0});
        mc.mask_bits.fill(~uint64_t{0});
        tmpl.mask_codes.push_back(mc);
    }
    return tmpl;
}

// ===== SimpleHammingDistanceMatcher tests =====

TEST(SimpleHammingDistanceMatcher, IdenticalZero) {
    SimpleHammingDistanceMatcher matcher;
    auto probe = make_uniform_template(0);
    auto gallery = make_uniform_template(0);

    auto result = matcher.run(probe, gallery);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_DOUBLE_EQ(result.value().distance, 0.0);
    EXPECT_EQ(result.value().best_rotation, 0);
}

TEST(SimpleHammingDistanceMatcher, IdenticalOnes) {
    SimpleHammingDistanceMatcher matcher;
    auto probe = make_uniform_template(~uint64_t{0});
    auto gallery = make_uniform_template(~uint64_t{0});

    auto result = matcher.run(probe, gallery);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result.value().distance, 0.0);
}

TEST(SimpleHammingDistanceMatcher, OppositeMax) {
    SimpleHammingDistanceMatcher::Params p;
    p.rotation_shift = 0;  // no rotation
    SimpleHammingDistanceMatcher matcher(p);

    auto probe = make_uniform_template(0);
    auto gallery = make_uniform_template(~uint64_t{0});

    auto result = matcher.run(probe, gallery);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result.value().distance, 1.0);
}

TEST(SimpleHammingDistanceMatcher, EmptyCodesFail) {
    SimpleHammingDistanceMatcher matcher;
    IrisTemplate empty;

    auto result = matcher.run(empty, empty);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::MatchingFailed);
}

TEST(SimpleHammingDistanceMatcher, EmptyMask) {
    SimpleHammingDistanceMatcher matcher;
    // Codes differ but mask is all zero → no valid bits → HD defaults to 1.0
    auto probe = make_uniform_template(0, 0);
    auto gallery = make_uniform_template(~uint64_t{0}, 0);

    auto result = matcher.run(probe, gallery);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result.value().distance, 1.0);
}

TEST(SimpleHammingDistanceMatcher, PartialMask) {
    SimpleHammingDistanceMatcher::Params p;
    p.rotation_shift = 0;
    SimpleHammingDistanceMatcher matcher(p);

    // Probe: code = all 1s, mask = 0x00FF...FF (lower 56 bits of each word)
    // Gallery: code = all 0s, mask = all 1s
    // Combined mask = probe mask = 0x00FF...FF
    // XOR = all 1s, masked = 56 set bits per word → HD = 56/56 = 1.0
    auto probe = make_uniform_template(~uint64_t{0}, 0x00FFFFFFFFFFFFFF);
    auto gallery = make_uniform_template(0, ~uint64_t{0});

    auto result = matcher.run(probe, gallery);
    ASSERT_TRUE(result.has_value());
    // All unmasked bits differ → HD should be 1.0
    EXPECT_DOUBLE_EQ(result.value().distance, 1.0);
}

TEST(SimpleHammingDistanceMatcher, HalfBitsDiffer) {
    SimpleHammingDistanceMatcher::Params p;
    p.rotation_shift = 0;
    SimpleHammingDistanceMatcher matcher(p);

    // Alternating bits: 0xAAAA... vs 0xFFFF...
    // XOR = 0x5555... → 32 bits per word out of 64
    auto probe = make_uniform_template(0xAAAAAAAAAAAAAAAA);
    auto gallery = make_uniform_template(~uint64_t{0});

    auto result = matcher.run(probe, gallery);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result.value().distance, 0.5);
}

TEST(SimpleHammingDistanceMatcher, RotationFindsMin) {
    // Create a probe that is a shifted version of the gallery.
    // The matcher should find the rotation that minimises the distance.
    SimpleHammingDistanceMatcher::Params p;
    p.rotation_shift = 5;
    SimpleHammingDistanceMatcher matcher(p);

    // Gallery: specific bit pattern
    IrisTemplate gallery;
    {
        PackedIrisCode ic;
        ic.mask_bits.fill(~uint64_t{0});
        // Put a recognisable pattern in row 0
        for (size_t w = 0; w < PackedIrisCode::kNumWords; ++w)
            ic.code_bits[w] = (w % 3 == 0) ? 0xDEADBEEFCAFEBABE : 0x1234567890ABCDEF;
        gallery.iris_codes.push_back(ic);
        PackedIrisCode mc;
        mc.code_bits.fill(~uint64_t{0});
        mc.mask_bits.fill(~uint64_t{0});
        gallery.mask_codes.push_back(mc);
    }

    // Probe: gallery rotated by 3 columns
    IrisTemplate probe;
    {
        auto rotated = gallery.iris_codes[0].rotate(3);
        probe.iris_codes.push_back(rotated);
        PackedIrisCode mc;
        mc.code_bits.fill(~uint64_t{0});
        mc.mask_bits.fill(~uint64_t{0});
        probe.mask_codes.push_back(mc);
    }

    auto result = matcher.run(probe, gallery);
    ASSERT_TRUE(result.has_value());
    // The best rotation should be 3 (probe.rotate(-3) == gallery, but matcher
    // tries probe.rotate(shift) against gallery, so shift = -3 gives 0 distance.
    // Actually: matcher rotates probe by `shift`, so probe.rotate(-3) undoes the
    // rotation of +3, matching gallery exactly.
    EXPECT_DOUBLE_EQ(result.value().distance, 0.0);
    EXPECT_EQ(result.value().best_rotation, -3);
}

TEST(SimpleHammingDistanceMatcher, RotationShiftZero) {
    SimpleHammingDistanceMatcher::Params p;
    p.rotation_shift = 0;
    SimpleHammingDistanceMatcher matcher(p);

    auto probe = make_uniform_template(0);
    auto gallery = make_uniform_template(0);

    auto result = matcher.run(probe, gallery);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().best_rotation, 0);
}

TEST(SimpleHammingDistanceMatcher, NormalizedHD) {
    SimpleHammingDistanceMatcher::Params p;
    p.rotation_shift = 0;
    p.normalise = true;
    p.norm_mean = 0.45;
    p.norm_gradient = 0.00005;
    SimpleHammingDistanceMatcher matcher(p);

    auto probe = make_uniform_template(0xAAAAAAAAAAAAAAAA);
    auto gallery = make_uniform_template(~uint64_t{0});

    auto result = matcher.run(probe, gallery);
    ASSERT_TRUE(result.has_value());

    // Raw HD = 0.5, mask_bits = 8192
    // norm = max(0, 0.45 - (0.45 - 0.5) * (0.00005 * 8192 + 0.5))
    //      = max(0, 0.45 - (-0.05) * (0.4096 + 0.5))
    //      = max(0, 0.45 + 0.05 * 0.9096)
    //      = max(0, 0.45 + 0.04548) = 0.49548
    double expected = 0.45 - (0.45 - 0.5) * (0.00005 * 8192.0 + 0.5);
    EXPECT_NEAR(result.value().distance, expected, 1e-10);
}

TEST(SimpleHammingDistanceMatcher, NormClampZero) {
    SimpleHammingDistanceMatcher::Params p;
    p.rotation_shift = 0;
    p.normalise = true;
    p.norm_mean = 0.45;
    p.norm_gradient = 0.00005;
    SimpleHammingDistanceMatcher matcher(p);

    // Identical → raw_hd = 0
    // norm = max(0, 0.45 - (0.45 - 0) * (0.00005 * 8192 + 0.5))
    //      = max(0, 0.45 - 0.45 * 0.9096) = max(0, 0.45 - 0.40932) = 0.04068
    auto probe = make_uniform_template(0);
    auto gallery = make_uniform_template(0);

    auto result = matcher.run(probe, gallery);
    ASSERT_TRUE(result.has_value());

    double expected = std::max(0.0, 0.45 - 0.45 * (0.00005 * 8192.0 + 0.5));
    EXPECT_NEAR(result.value().distance, expected, 1e-10);
    EXPECT_GT(result.value().distance, 0.0);
}

TEST(SimpleHammingDistanceMatcher, NodeParams) {
    std::unordered_map<std::string, std::string> np;
    np["rotation_shift"] = "10";
    np["normalise"] = "true";
    np["norm_mean"] = "0.42";
    np["norm_gradient"] = "0.0001";

    SimpleHammingDistanceMatcher matcher(np);
    EXPECT_EQ(matcher.params().rotation_shift, 10);
    EXPECT_TRUE(matcher.params().normalise);
    EXPECT_NEAR(matcher.params().norm_mean, 0.42, 1e-10);
    EXPECT_NEAR(matcher.params().norm_gradient, 0.0001, 1e-10);
}

// ===== HammingDistanceMatcher tests =====

TEST(HammingDistanceMatcher, IdenticalZero) {
    HammingDistanceMatcher matcher;
    auto probe = make_uniform_template(0);
    auto gallery = make_uniform_template(0);

    auto result = matcher.run(probe, gallery);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    // Normalised HD of 0 raw is still > 0 because of normalisation
    EXPECT_GE(result.value().distance, 0.0);
}

TEST(HammingDistanceMatcher, SeparateHalfEnabled) {
    HammingDistanceMatcher::Params p;
    p.rotation_shift = 0;
    p.normalise = false;
    p.separate_half_matching = true;
    HammingDistanceMatcher matcher(p);

    // Upper half identical, lower half opposite
    IrisTemplate probe, gallery;
    {
        PackedIrisCode pic, gic;
        pic.mask_bits.fill(~uint64_t{0});
        gic.mask_bits.fill(~uint64_t{0});
        // Upper half (words 0..63): both zero → HD_upper = 0
        for (size_t w = 0; w < PackedIrisCode::kNumWords / 2; ++w) {
            pic.code_bits[w] = 0;
            gic.code_bits[w] = 0;
        }
        // Lower half (words 64..127): probe = 0, gallery = all 1s → HD_lower = 1.0
        for (size_t w = PackedIrisCode::kNumWords / 2; w < PackedIrisCode::kNumWords; ++w) {
            pic.code_bits[w] = 0;
            gic.code_bits[w] = ~uint64_t{0};
        }
        probe.iris_codes.push_back(pic);
        gallery.iris_codes.push_back(gic);

        PackedIrisCode mc;
        mc.code_bits.fill(~uint64_t{0});
        mc.mask_bits.fill(~uint64_t{0});
        probe.mask_codes.push_back(mc);
        gallery.mask_codes.push_back(mc);
    }

    auto result = matcher.run(probe, gallery);
    ASSERT_TRUE(result.has_value());
    // Average of 0.0 and 1.0
    EXPECT_DOUBLE_EQ(result.value().distance, 0.5);
}

TEST(HammingDistanceMatcher, SeparateHalfDisabled) {
    HammingDistanceMatcher::Params p;
    p.rotation_shift = 0;
    p.normalise = false;
    p.separate_half_matching = false;
    HammingDistanceMatcher matcher(p);

    // Same setup as above
    IrisTemplate probe, gallery;
    {
        PackedIrisCode pic, gic;
        pic.mask_bits.fill(~uint64_t{0});
        gic.mask_bits.fill(~uint64_t{0});
        for (size_t w = 0; w < PackedIrisCode::kNumWords / 2; ++w) {
            pic.code_bits[w] = 0;
            gic.code_bits[w] = 0;
        }
        for (size_t w = PackedIrisCode::kNumWords / 2; w < PackedIrisCode::kNumWords; ++w) {
            pic.code_bits[w] = 0;
            gic.code_bits[w] = ~uint64_t{0};
        }
        probe.iris_codes.push_back(pic);
        gallery.iris_codes.push_back(gic);

        PackedIrisCode mc;
        mc.code_bits.fill(~uint64_t{0});
        mc.mask_bits.fill(~uint64_t{0});
        probe.mask_codes.push_back(mc);
        gallery.mask_codes.push_back(mc);
    }

    auto result = matcher.run(probe, gallery);
    ASSERT_TRUE(result.has_value());
    // Without separate half: 4096 bits differ out of 8192 → 0.5
    // Same result here because the halves are equal in size, but the computation
    // path is different (whole code at once vs average of halves)
    EXPECT_DOUBLE_EQ(result.value().distance, 0.5);
}

TEST(HammingDistanceMatcher, MultipleWavelets) {
    HammingDistanceMatcher::Params p;
    p.rotation_shift = 0;
    p.normalise = false;
    p.separate_half_matching = false;
    HammingDistanceMatcher matcher(p);

    // Two wavelet pairs: first identical (HD=0), second opposite (HD=1)
    // Overall: 8192 differing bits out of 16384 → HD = 0.5
    auto probe = make_two_wavelet_template(0, 0);
    auto gallery = make_two_wavelet_template(0, ~uint64_t{0});

    auto result = matcher.run(probe, gallery);
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result.value().distance, 0.5);
}

TEST(HammingDistanceMatcher, NodeParams) {
    std::unordered_map<std::string, std::string> np;
    np["rotation_shift"] = "20";
    np["normalise"] = "false";
    np["norm_mean"] = "0.50";
    np["norm_gradient"] = "0.0002";
    np["separate_half_matching"] = "false";

    HammingDistanceMatcher matcher(np);
    EXPECT_EQ(matcher.params().rotation_shift, 20);
    EXPECT_FALSE(matcher.params().normalise);
    EXPECT_NEAR(matcher.params().norm_mean, 0.50, 1e-10);
    EXPECT_NEAR(matcher.params().norm_gradient, 0.0002, 1e-10);
    EXPECT_FALSE(matcher.params().separate_half_matching);
}
