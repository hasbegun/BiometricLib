#ifdef IRIS_HAS_FHE

#include <iris/crypto/encrypted_matcher.hpp>

#include <bit>
#include <cstdint>
#include <random>

#include <gtest/gtest.h>

// Suppress warnings from OpenFHE template instantiations in test code
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wnull-dereference"
#pragma GCC diagnostic ignored "-Wformat=2"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif

namespace iris::test {

namespace {

/// Create an FHEContext with keys.
FHEContext make_ctx() {
    auto ctx = FHEContext::create();
    EXPECT_TRUE(ctx.has_value());
    auto kr = ctx->generate_keys();
    EXPECT_TRUE(kr.has_value());
    return std::move(*ctx);
}

/// Compute plaintext Hamming distance for a single PackedIrisCode pair.
double plaintext_hd(const PackedIrisCode& probe,
                    const PackedIrisCode& gallery) {
    size_t num = 0;
    size_t den = 0;
    for (size_t w = 0; w < PackedIrisCode::kNumWords; ++w) {
        uint64_t mask = probe.mask_bits[w] & gallery.mask_bits[w];
        uint64_t diff = (probe.code_bits[w] ^ gallery.code_bits[w]) & mask;
        num += static_cast<size_t>(std::popcount(diff));
        den += static_cast<size_t>(std::popcount(mask));
    }
    if (den == 0) return 1.0;
    return static_cast<double>(num) / static_cast<double>(den);
}

/// Compute plaintext HD with rotation, return {min_hd, best_shift}.
std::pair<double, int> plaintext_hd_rotated(
    const PackedIrisCode& probe,
    const PackedIrisCode& gallery,
    int rotation_shift) {
    double best_hd = 1.0;
    int best_shift = 0;
    for (int abs_s = 0; abs_s <= rotation_shift; ++abs_s) {
        for (int sign : {1, -1}) {
            if (abs_s == 0 && sign == -1) continue;
            int s = abs_s * sign;
            auto rotated = probe.rotate(s);
            double hd = plaintext_hd(rotated, gallery);
            if (hd < best_hd) {
                best_hd = hd;
                best_shift = s;
            }
        }
    }
    return {best_hd, best_shift};
}

}  // namespace

// --- Step 7.4 tests ---

TEST(EncryptedMatcher, IdenticalTemplatesHDZero) {
    auto ctx = make_ctx();

    PackedIrisCode code;
    std::mt19937_64 rng(1);
    for (auto& w : code.code_bits) w = rng();
    for (auto& w : code.mask_bits) w = ~uint64_t{0};  // all valid

    auto hd = EncryptedMatcher::match(ctx, code, code);
    ASSERT_TRUE(hd.has_value()) << hd.error().message;
    EXPECT_DOUBLE_EQ(*hd, 0.0);
}

TEST(EncryptedMatcher, OppositeTemplatesHDOne) {
    auto ctx = make_ctx();

    PackedIrisCode a;
    PackedIrisCode b;
    for (auto& w : a.mask_bits) w = ~uint64_t{0};
    for (auto& w : b.mask_bits) w = ~uint64_t{0};
    // a = all zeros, b = all ones
    for (auto& w : b.code_bits) w = ~uint64_t{0};

    auto hd = EncryptedMatcher::match(ctx, a, b);
    ASSERT_TRUE(hd.has_value()) << hd.error().message;
    EXPECT_DOUBLE_EQ(*hd, 1.0);
}

TEST(EncryptedMatcher, HalfDifferentTemplates) {
    auto ctx = make_ctx();

    PackedIrisCode a;
    PackedIrisCode b;
    for (auto& w : a.mask_bits) w = ~uint64_t{0};
    for (auto& w : b.mask_bits) w = ~uint64_t{0};
    // a = all zeros
    // b = alternating words: half set, half not
    for (size_t i = 0; i < PackedIrisCode::kNumWords; ++i) {
        b.code_bits[i] = (i % 2 == 0) ? ~uint64_t{0} : uint64_t{0};
    }

    auto hd = EncryptedMatcher::match(ctx, a, b);
    ASSERT_TRUE(hd.has_value()) << hd.error().message;
    EXPECT_NEAR(*hd, 0.5, 1e-10);
}

TEST(EncryptedMatcher, MatchesPlaintextHD) {
    auto ctx = make_ctx();

    PackedIrisCode probe;
    PackedIrisCode gallery;
    std::mt19937_64 rng(77);
    for (auto& w : probe.code_bits) w = rng();
    for (auto& w : probe.mask_bits) w = rng();
    for (auto& w : gallery.code_bits) w = rng();
    for (auto& w : gallery.mask_bits) w = rng();

    double expected = plaintext_hd(probe, gallery);

    auto encrypted_hd = EncryptedMatcher::match(ctx, probe, gallery);
    ASSERT_TRUE(encrypted_hd.has_value()) << encrypted_hd.error().message;

    EXPECT_NEAR(*encrypted_hd, expected, 1e-10);
}

TEST(EncryptedMatcher, PartialMaskMatchesPlaintext) {
    auto ctx = make_ctx();

    PackedIrisCode probe;
    PackedIrisCode gallery;
    std::mt19937_64 rng(33);

    for (auto& w : probe.code_bits) w = rng();
    for (auto& w : gallery.code_bits) w = rng();

    // Probe mask: upper half valid, lower half invalid
    for (size_t i = 0; i < PackedIrisCode::kNumWords; ++i) {
        probe.mask_bits[i] = (i < PackedIrisCode::kNumWords / 2)
                                 ? ~uint64_t{0}
                                 : uint64_t{0};
    }
    // Gallery mask: all valid
    for (auto& w : gallery.mask_bits) w = ~uint64_t{0};

    double expected = plaintext_hd(probe, gallery);
    auto encrypted_hd = EncryptedMatcher::match(ctx, probe, gallery);
    ASSERT_TRUE(encrypted_hd.has_value()) << encrypted_hd.error().message;
    EXPECT_NEAR(*encrypted_hd, expected, 1e-10);
}

TEST(EncryptedMatcher, EmptyMaskReturnsOne) {
    auto ctx = make_ctx();

    PackedIrisCode a;
    PackedIrisCode b;
    // Both masks all zeros → no valid bits → HD = 1.0

    auto hd = EncryptedMatcher::match(ctx, a, b);
    ASSERT_TRUE(hd.has_value()) << hd.error().message;
    EXPECT_DOUBLE_EQ(*hd, 1.0);
}

TEST(EncryptedMatcher, RandomParity) {
    auto ctx = make_ctx();
    std::mt19937_64 rng(123);

    for (int trial = 0; trial < 5; ++trial) {
        PackedIrisCode probe;
        PackedIrisCode gallery;
        for (auto& w : probe.code_bits) w = rng();
        for (auto& w : probe.mask_bits) w = rng() | 0x0101010101010101ULL;
        for (auto& w : gallery.code_bits) w = rng();
        for (auto& w : gallery.mask_bits) w = rng() | 0x0101010101010101ULL;

        double expected = plaintext_hd(probe, gallery);
        auto encrypted_hd = EncryptedMatcher::match(ctx, probe, gallery);
        ASSERT_TRUE(encrypted_hd.has_value()) << encrypted_hd.error().message;
        EXPECT_NEAR(*encrypted_hd, expected, 1e-10) << "trial " << trial;
    }
}

TEST(EncryptedMatcher, ConvenienceMatchWorks) {
    auto ctx = make_ctx();

    PackedIrisCode probe;
    PackedIrisCode gallery;
    std::mt19937_64 rng(55);
    for (auto& w : probe.code_bits) w = rng();
    for (auto& w : probe.mask_bits) w = ~uint64_t{0};
    for (auto& w : gallery.code_bits) w = rng();
    for (auto& w : gallery.mask_bits) w = ~uint64_t{0};

    // Convenience match
    auto hd1 = EncryptedMatcher::match(ctx, probe, gallery);
    ASSERT_TRUE(hd1.has_value());

    // Manual: encrypt + match + decrypt
    auto ep = EncryptedTemplate::encrypt(ctx, probe);
    auto eg = EncryptedTemplate::encrypt(ctx, gallery);
    ASSERT_TRUE(ep.has_value());
    ASSERT_TRUE(eg.has_value());
    auto res = EncryptedMatcher::match_encrypted(ctx, *ep, *eg);
    ASSERT_TRUE(res.has_value());
    auto hd2 = EncryptedMatcher::decrypt_result(ctx, *res);
    ASSERT_TRUE(hd2.has_value());

    EXPECT_NEAR(*hd1, *hd2, 1e-10);
}

// --- Step 7.5 rotation tests ---

TEST(EncryptedMatcher, RotationIdenticalTemplates) {
    auto ctx = make_ctx();

    PackedIrisCode code;
    std::mt19937_64 rng(7);
    for (auto& w : code.code_bits) w = rng();
    for (auto& w : code.mask_bits) w = ~uint64_t{0};

    auto result = EncryptedMatcher::match_with_rotation(ctx, code, code, 3);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_DOUBLE_EQ(result->distance, 0.0);
}

TEST(EncryptedMatcher, RotationShiftZeroMatchesNoRotation) {
    auto ctx = make_ctx();

    PackedIrisCode probe;
    PackedIrisCode gallery;
    std::mt19937_64 rng(88);
    for (auto& w : probe.code_bits) w = rng();
    for (auto& w : probe.mask_bits) w = ~uint64_t{0};
    for (auto& w : gallery.code_bits) w = rng();
    for (auto& w : gallery.mask_bits) w = ~uint64_t{0};

    auto no_rot = EncryptedMatcher::match(ctx, probe, gallery);
    auto with_rot = EncryptedMatcher::match_with_rotation(ctx, probe, gallery, 0);
    ASSERT_TRUE(no_rot.has_value());
    ASSERT_TRUE(with_rot.has_value());
    EXPECT_NEAR(*no_rot, with_rot->distance, 1e-10);
}

TEST(EncryptedMatcher, RotationFindsCorrectShift) {
    auto ctx = make_ctx();

    // Create a probe that is a shifted version of the gallery
    PackedIrisCode gallery;
    std::mt19937_64 rng(42);
    for (auto& w : gallery.code_bits) w = rng();
    for (auto& w : gallery.mask_bits) w = ~uint64_t{0};

    // Probe = gallery shifted by +3 columns
    auto probe = gallery.rotate(3);

    // Encrypted rotation should find HD ≈ 0 at shift = -3
    auto result = EncryptedMatcher::match_with_rotation(ctx, probe, gallery, 5);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_NEAR(result->distance, 0.0, 1e-10);
    EXPECT_EQ(result->best_rotation, -3);
}

TEST(EncryptedMatcher, RotationMatchesPlaintext) {
    auto ctx = make_ctx();

    PackedIrisCode probe;
    PackedIrisCode gallery;
    std::mt19937_64 rng(99);
    for (auto& w : probe.code_bits) w = rng();
    for (auto& w : probe.mask_bits) w = rng() | 0x0101010101010101ULL;
    for (auto& w : gallery.code_bits) w = rng();
    for (auto& w : gallery.mask_bits) w = rng() | 0x0101010101010101ULL;

    constexpr int kShift = 3;
    auto [plain_hd, plain_rot] = plaintext_hd_rotated(probe, gallery, kShift);

    auto enc_result =
        EncryptedMatcher::match_with_rotation(ctx, probe, gallery, kShift);
    ASSERT_TRUE(enc_result.has_value()) << enc_result.error().message;

    EXPECT_NEAR(enc_result->distance, plain_hd, 1e-10);
    EXPECT_EQ(enc_result->best_rotation, plain_rot);
}

}  // namespace iris::test

#pragma GCC diagnostic pop

#endif  // IRIS_HAS_FHE
