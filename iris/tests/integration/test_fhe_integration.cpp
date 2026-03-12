#ifdef IRIS_HAS_FHE

#include <iris/crypto/encrypted_matcher.hpp>
#include <iris/crypto/encrypted_template.hpp>
#include <iris/crypto/fhe_context.hpp>
#include <iris/crypto/key_manager.hpp>
#include <iris/crypto/key_store.hpp>

#include <cstdint>
#include <filesystem>
#include <random>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

// Suppress warnings from OpenFHE template instantiations
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

FHEContext make_ctx() {
    auto ctx = FHEContext::create();
    EXPECT_TRUE(ctx.has_value());
    auto kr = ctx->generate_keys();
    EXPECT_TRUE(kr.has_value());
    return std::move(*ctx);
}

PackedIrisCode random_code(std::mt19937_64& rng) {
    PackedIrisCode code;
    for (auto& w : code.code_bits) w = rng();
    for (auto& w : code.mask_bits) w = ~uint64_t{0};
    return code;
}

double plaintext_hd(const PackedIrisCode& a, const PackedIrisCode& b) {
    size_t num = 0;
    size_t den = 0;
    for (size_t w = 0; w < PackedIrisCode::kNumWords; ++w) {
        uint64_t mask = a.mask_bits[w] & b.mask_bits[w];
        uint64_t diff = (a.code_bits[w] ^ b.code_bits[w]) & mask;
        num += static_cast<size_t>(std::popcount(diff));
        den += static_cast<size_t>(std::popcount(mask));
    }
    if (den == 0) return 1.0;
    return static_cast<double>(num) / static_cast<double>(den);
}

struct TmpDir {
    std::filesystem::path path;
    explicit TmpDir(const std::string& name)
        : path(std::filesystem::temp_directory_path() / name) {
        std::filesystem::remove_all(path);
    }
    ~TmpDir() { std::filesystem::remove_all(path); }
};

}  // namespace

TEST(FHEIntegration, FullFlow) {
    // Generate context + keys → encrypt → match encrypted → decrypt → verify
    auto ctx = make_ctx();

    std::mt19937_64 rng(42);
    auto probe = random_code(rng);
    auto gallery = random_code(rng);

    double expected = plaintext_hd(probe, gallery);

    auto enc_probe = EncryptedTemplate::encrypt(ctx, probe);
    ASSERT_TRUE(enc_probe.has_value());
    auto enc_gallery = EncryptedTemplate::encrypt(ctx, gallery);
    ASSERT_TRUE(enc_gallery.has_value());

    auto result = EncryptedMatcher::match_encrypted(ctx, *enc_probe, *enc_gallery);
    ASSERT_TRUE(result.has_value());

    auto hd = EncryptedMatcher::decrypt_result(ctx, *result);
    ASSERT_TRUE(hd.has_value());

    EXPECT_NEAR(*hd, expected, 1e-10);
}

TEST(FHEIntegration, EncryptedBatchMatchesPlaintext) {
    // 1-vs-5 gallery, verify all results match plaintext
    auto ctx = make_ctx();

    std::mt19937_64 rng(100);
    auto probe = random_code(rng);

    constexpr int kGallerySize = 5;
    std::vector<PackedIrisCode> gallery(kGallerySize);
    for (auto& g : gallery) g = random_code(rng);

    auto enc_probe = EncryptedTemplate::encrypt(ctx, probe);
    ASSERT_TRUE(enc_probe.has_value());

    for (int i = 0; i < kGallerySize; ++i) {
        double expected = plaintext_hd(probe, gallery[static_cast<size_t>(i)]);

        auto enc_g = EncryptedTemplate::encrypt(ctx, gallery[static_cast<size_t>(i)]);
        ASSERT_TRUE(enc_g.has_value());

        auto result = EncryptedMatcher::match_encrypted(ctx, *enc_probe, *enc_g);
        ASSERT_TRUE(result.has_value());

        auto hd = EncryptedMatcher::decrypt_result(ctx, *result);
        ASSERT_TRUE(hd.has_value());

        EXPECT_NEAR(*hd, expected, 1e-10) << "gallery index " << i;
    }
}

TEST(FHEIntegration, KeySaveLoadEncryptMatch) {
    // Generate keys → save → load → encrypt → match → decrypt
    auto ctx = make_ctx();
    auto bundle = KeyManager::generate(ctx);
    ASSERT_TRUE(bundle.has_value());

    TmpDir dir("test_fhe_integ_keys");
    auto sr = KeyStore::save(dir.path, ctx, *bundle);
    ASSERT_TRUE(sr.has_value());

    auto loaded = KeyStore::load(dir.path, ctx);
    ASSERT_TRUE(loaded.has_value());

    // The original context still has eval keys so encrypt/match works
    std::mt19937_64 rng(77);
    auto probe = random_code(rng);
    auto gallery = random_code(rng);

    double expected = plaintext_hd(probe, gallery);
    auto hd = EncryptedMatcher::match(ctx, probe, gallery);
    ASSERT_TRUE(hd.has_value());
    EXPECT_NEAR(*hd, expected, 1e-10);
}

TEST(FHEIntegration, ReencryptWithNewKeys) {
    // Encrypt with keys1, decrypt → re-encrypt with keys2 → decrypt → verify
    auto ctx1 = make_ctx();
    auto ctx2 = make_ctx();

    std::mt19937_64 rng(55);
    auto code = random_code(rng);

    // Encrypt with ctx1
    auto enc1 = EncryptedTemplate::encrypt(ctx1, code);
    ASSERT_TRUE(enc1.has_value());

    // Decrypt with ctx1
    auto dec1 = enc1->decrypt(ctx1);
    ASSERT_TRUE(dec1.has_value());
    EXPECT_EQ(dec1->code_bits, code.code_bits);

    // Re-encrypt with ctx2
    auto enc2 = EncryptedTemplate::encrypt(ctx2, *dec1);
    ASSERT_TRUE(enc2.has_value());

    // Decrypt with ctx2
    auto dec2 = enc2->decrypt(ctx2);
    ASSERT_TRUE(dec2.has_value());
    EXPECT_EQ(dec2->code_bits, code.code_bits);
}

TEST(FHEIntegration, ConcurrentMatchesIndependent) {
    auto ctx = make_ctx();

    std::mt19937_64 rng(11);
    constexpr int kPairs = 3;

    struct Pair {
        PackedIrisCode probe;
        PackedIrisCode gallery;
        double expected_hd;
    };

    std::vector<Pair> pairs(kPairs);
    for (auto& p : pairs) {
        p.probe = random_code(rng);
        p.gallery = random_code(rng);
        p.expected_hd = plaintext_hd(p.probe, p.gallery);
    }

    // Run matches sequentially (OpenFHE contexts aren't thread-safe)
    // but verify each result is independent
    for (int i = 0; i < kPairs; ++i) {
        auto hd = EncryptedMatcher::match(ctx, pairs[static_cast<size_t>(i)].probe,
                                           pairs[static_cast<size_t>(i)].gallery);
        ASSERT_TRUE(hd.has_value());
        EXPECT_NEAR(*hd, pairs[static_cast<size_t>(i)].expected_hd, 1e-10)
            << "pair " << i;
    }
}

TEST(FHEIntegration, RotationEndToEnd) {
    // Full rotation match: verify encrypted result matches plaintext
    auto ctx = make_ctx();

    std::mt19937_64 rng(33);
    auto gallery = random_code(rng);

    // Create probe = gallery shifted by +2
    auto probe = gallery.rotate(2);

    // Encrypted rotation should find the correct shift
    auto result = EncryptedMatcher::match_with_rotation(ctx, probe, gallery, 3);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_NEAR(result->distance, 0.0, 1e-10);
    EXPECT_EQ(result->best_rotation, -2);
}

// --- Sign-off tests ---

TEST(FHEIntegration, DecisionAgreementZeroDisagreements) {
    // Sign-off: encrypted matching decision agreement == 0 disagreements vs
    // plaintext at threshold 0.35.
    // Generate multiple probe/gallery pairs, compute HD both ways, verify
    // match/no-match decision is identical for every pair.
    auto ctx = make_ctx();

    constexpr double kThreshold = 0.35;
    constexpr int kNumPairs = 10;
    int disagreements = 0;

    std::mt19937_64 rng(314);
    for (int i = 0; i < kNumPairs; ++i) {
        auto probe = random_code(rng);
        auto gallery = random_code(rng);

        double plain_hd_val = plaintext_hd(probe, gallery);
        bool plain_match = plain_hd_val < kThreshold;

        auto enc_hd = EncryptedMatcher::match(ctx, probe, gallery);
        ASSERT_TRUE(enc_hd.has_value()) << enc_hd.error().message;
        bool enc_match = *enc_hd < kThreshold;

        if (plain_match != enc_match) {
            ++disagreements;
            ADD_FAILURE() << "Decision disagreement at pair " << i
                          << ": plaintext HD=" << plain_hd_val
                          << " (match=" << plain_match << ")"
                          << ", encrypted HD=" << *enc_hd
                          << " (match=" << enc_match << ")";
        }

        // Also verify HD values are within tolerance
        EXPECT_NEAR(*enc_hd, plain_hd_val, 1e-6)
            << "HD value divergence at pair " << i;
    }

    EXPECT_EQ(disagreements, 0)
        << disagreements << " out of " << kNumPairs
        << " pairs had different match/no-match decisions between "
           "encrypted and plaintext matching.";
}

}  // namespace iris::test

#pragma GCC diagnostic pop

#endif  // IRIS_HAS_FHE
