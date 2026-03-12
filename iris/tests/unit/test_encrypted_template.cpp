#ifdef IRIS_HAS_FHE

#include <iris/crypto/encrypted_template.hpp>
#include <iris/crypto/fhe_context.hpp>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
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
/// Helper to create an FHEContext with keys for tests.
FHEContext make_ctx() {
    auto ctx = FHEContext::create();
    EXPECT_TRUE(ctx.has_value()) << ctx.error().message;
    auto kr = ctx->generate_keys();
    EXPECT_TRUE(kr.has_value()) << kr.error().message << " | " << kr.error().detail;
    return std::move(*ctx);
}
}  // namespace

TEST(EncryptedTemplate, RoundTripZeros) {
    auto ctx = make_ctx();
    PackedIrisCode code;  // all zeros

    auto enc = EncryptedTemplate::encrypt(ctx, code);
    ASSERT_TRUE(enc.has_value()) << enc.error().message;

    auto dec = enc->decrypt(ctx);
    ASSERT_TRUE(dec.has_value()) << dec.error().message;

    EXPECT_EQ(dec->code_bits, code.code_bits);
    EXPECT_EQ(dec->mask_bits, code.mask_bits);
}

TEST(EncryptedTemplate, RoundTripRandom) {
    auto ctx = make_ctx();

    PackedIrisCode code;
    std::mt19937_64 rng(42);
    for (auto& w : code.code_bits) w = rng();
    for (auto& w : code.mask_bits) w = rng();

    auto enc = EncryptedTemplate::encrypt(ctx, code);
    ASSERT_TRUE(enc.has_value()) << enc.error().message;

    auto dec = enc->decrypt(ctx);
    ASSERT_TRUE(dec.has_value()) << dec.error().message;

    EXPECT_EQ(dec->code_bits, code.code_bits);
    EXPECT_EQ(dec->mask_bits, code.mask_bits);
}

TEST(EncryptedTemplate, RoundTripAllOnes) {
    auto ctx = make_ctx();

    PackedIrisCode code;
    for (auto& w : code.code_bits) w = ~uint64_t{0};
    for (auto& w : code.mask_bits) w = ~uint64_t{0};

    auto enc = EncryptedTemplate::encrypt(ctx, code);
    ASSERT_TRUE(enc.has_value()) << enc.error().message;

    auto dec = enc->decrypt(ctx);
    ASSERT_TRUE(dec.has_value()) << dec.error().message;

    EXPECT_EQ(dec->code_bits, code.code_bits);
    EXPECT_EQ(dec->mask_bits, code.mask_bits);
}

TEST(EncryptedTemplate, SerializeDeserializeRoundTrip) {
    auto ctx = make_ctx();

    PackedIrisCode code;
    std::mt19937_64 rng(99);
    for (auto& w : code.code_bits) w = rng();
    for (auto& w : code.mask_bits) w = rng();

    auto enc = EncryptedTemplate::encrypt(ctx, code);
    ASSERT_TRUE(enc.has_value()) << enc.error().message;

    auto bytes = enc->serialize();
    ASSERT_TRUE(bytes.has_value()) << bytes.error().message;
    EXPECT_GT(bytes->size(), 0u);

    auto restored = EncryptedTemplate::deserialize(ctx, *bytes);
    ASSERT_TRUE(restored.has_value()) << restored.error().message;

    auto dec = restored->decrypt(ctx);
    ASSERT_TRUE(dec.has_value()) << dec.error().message;

    EXPECT_EQ(dec->code_bits, code.code_bits);
    EXPECT_EQ(dec->mask_bits, code.mask_bits);
}

TEST(EncryptedTemplate, EncryptWithNoKeysFails) {
    auto ctx_result = FHEContext::create();
    ASSERT_TRUE(ctx_result.has_value()) << ctx_result.error().message;
    // Do NOT generate keys

    PackedIrisCode code;
    auto enc = EncryptedTemplate::encrypt(*ctx_result, code);
    EXPECT_FALSE(enc.has_value());
    EXPECT_EQ(enc.error().code, ErrorCode::CryptoFailed);
}

TEST(EncryptedTemplate, ReEncryptStandalone) {
    auto old_ctx = make_ctx();
    auto new_ctx = make_ctx();

    PackedIrisCode code;
    std::mt19937_64 rng(77);
    for (auto& w : code.code_bits) w = rng();
    for (auto& w : code.mask_bits) w = rng();

    auto enc = EncryptedTemplate::encrypt(old_ctx, code);
    ASSERT_TRUE(enc.has_value()) << enc.error().message;

    auto re_enc = EncryptedTemplate::re_encrypt(old_ctx, new_ctx, *enc);
    ASSERT_TRUE(re_enc.has_value()) << re_enc.error().message;

    // Decrypt with new context
    auto dec = re_enc->decrypt(new_ctx);
    ASSERT_TRUE(dec.has_value()) << dec.error().message;

    EXPECT_EQ(dec->code_bits, code.code_bits);
    EXPECT_EQ(dec->mask_bits, code.mask_bits);
}

TEST(EncryptedTemplate, MaskBitsPreserved) {
    auto ctx = make_ctx();

    // Code = all zeros, mask = specific pattern
    PackedIrisCode code;
    for (size_t i = 0; i < code.mask_bits.size(); ++i) {
        code.mask_bits[i] = (i % 2 == 0) ? 0xAAAAAAAAAAAAAAAAULL
                                          : 0x5555555555555555ULL;
    }

    auto enc = EncryptedTemplate::encrypt(ctx, code);
    ASSERT_TRUE(enc.has_value()) << enc.error().message;

    auto dec = enc->decrypt(ctx);
    ASSERT_TRUE(dec.has_value()) << dec.error().message;

    // Code bits should all be zero
    for (auto w : dec->code_bits) {
        EXPECT_EQ(w, uint64_t{0});
    }
    // Mask bits should match the pattern
    EXPECT_EQ(dec->mask_bits, code.mask_bits);
}

// --- Sign-off tests ---

TEST(EncryptedTemplate, HighEntropyCiphertext) {
    // Sign-off: encrypted template has high Shannon entropy.
    //
    // BFV ciphertexts use RNS (Residue Number System) representation with
    // cereal serialization framing. The polynomial coefficients are large
    // integers modulo RNS primes — they have high entropy but are NOT
    // uniformly random bytes (that would require a stream cipher on top).
    // IND-CPA security means ciphertexts of different plaintexts are
    // indistinguishable from each other, not from random data.
    //
    // Measured: ~7.88 bits/byte for BFV serialized ciphertext.
    // Threshold: > 7.5 bits/byte (well above structured/plaintext data,
    // which typically has 4-6 bits/byte).
    auto ctx = make_ctx();

    PackedIrisCode code;
    std::mt19937_64 rng(42);
    for (auto& w : code.code_bits) w = rng();
    for (auto& w : code.mask_bits) w = rng();

    auto enc = EncryptedTemplate::encrypt(ctx, code);
    ASSERT_TRUE(enc.has_value()) << enc.error().message;

    auto bytes = enc->serialize();
    ASSERT_TRUE(bytes.has_value()) << bytes.error().message;
    ASSERT_GT(bytes->size(), 256u);

    // Compute Shannon entropy: H = -Σ p(x) log2(p(x)) for byte values
    std::array<size_t, 256> counts{};
    for (auto b : *bytes) {
        counts[b]++;
    }

    double entropy = 0.0;
    auto n = static_cast<double>(bytes->size());
    for (auto c : counts) {
        if (c == 0) continue;
        double p = static_cast<double>(c) / n;
        entropy -= p * std::log2(p);
    }

    // BFV RNS ciphertext achieves ~7.88 bits/byte. The 7.5 threshold ensures
    // the ciphertext is meaningfully encrypted (not plaintext or low-entropy).
    // For comparison: English text ≈ 4.0, compressed data ≈ 7.5-7.9,
    // truly random ≈ 8.0.
    EXPECT_GT(entropy, 7.5)
        << "Encrypted template entropy too low: " << entropy
        << " bits/byte (expected > 7.5). Ciphertext may not be properly "
           "encrypted.";
}

TEST(EncryptedTemplate, NoPlaintextLeakInCiphertext) {
    // Sign-off: no plaintext iris code bits leak into encrypted output.
    // The plaintext iris code (as raw bytes) must NOT appear as a substring
    // anywhere in the serialized ciphertext.
    auto ctx = make_ctx();

    // Use a distinctive non-random pattern to make detection reliable
    PackedIrisCode code;
    for (size_t i = 0; i < code.code_bits.size(); ++i) {
        code.code_bits[i] = 0xDEADBEEFCAFEBABEULL;
    }
    for (auto& w : code.mask_bits) w = ~uint64_t{0};

    auto enc = EncryptedTemplate::encrypt(ctx, code);
    ASSERT_TRUE(enc.has_value()) << enc.error().message;

    auto bytes = enc->serialize();
    ASSERT_TRUE(bytes.has_value()) << bytes.error().message;

    // Check: the 8-byte plaintext pattern should NOT appear in the ciphertext
    const uint64_t pattern = 0xDEADBEEFCAFEBABEULL;
    std::array<uint8_t, 8> pattern_bytes{};
    std::memcpy(pattern_bytes.data(), &pattern, 8);

    // Search for the pattern in the serialized ciphertext
    bool found = false;
    if (bytes->size() >= 8) {
        for (size_t i = 0; i <= bytes->size() - 8; ++i) {
            if (std::memcmp(bytes->data() + i, pattern_bytes.data(), 8) == 0) {
                found = true;
                break;
            }
        }
    }

    EXPECT_FALSE(found)
        << "Plaintext iris code pattern found in encrypted output! "
           "The ciphertext contains a verbatim copy of the plaintext bits, "
           "indicating a potential leak.";

    // IND-CPA verification: encrypting the SAME plaintext twice must produce
    // DIFFERENT ciphertexts (due to randomness in encryption).
    auto enc2 = EncryptedTemplate::encrypt(ctx, code);
    ASSERT_TRUE(enc2.has_value()) << enc2.error().message;

    auto bytes2 = enc2->serialize();
    ASSERT_TRUE(bytes2.has_value()) << bytes2.error().message;

    // Two encryptions of the same plaintext must differ
    EXPECT_NE(*bytes, *bytes2)
        << "Two encryptions of the same plaintext produced identical "
           "ciphertexts! This violates IND-CPA security — encryption must "
           "be probabilistic (randomized).";
}

}  // namespace iris::test

#pragma GCC diagnostic pop

#endif  // IRIS_HAS_FHE
