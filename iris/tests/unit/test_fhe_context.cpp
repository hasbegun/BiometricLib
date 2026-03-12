#ifdef IRIS_HAS_FHE

#include <iris/crypto/fhe_context.hpp>

#include <cstdint>
#include <vector>

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

TEST(FHEContext, CreateDefault) {
    auto result = FHEContext::create();
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_GT(result->slot_count(), 0u);
}

TEST(FHEContext, SlotCountIs8192) {
    auto result = FHEContext::create();
    ASSERT_TRUE(result.has_value()) << result.error().message;
    // ring_dim=16384 → slot_count = ring_dim / 2 = 8192
    EXPECT_EQ(result->slot_count(), 8192u);
}

TEST(FHEContext, NoKeysBeforeGeneration) {
    auto result = FHEContext::create();
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_FALSE(result->has_keys());
}

TEST(FHEContext, GenerateKeysSucceeds) {
    auto result = FHEContext::create();
    ASSERT_TRUE(result.has_value()) << result.error().message;

    auto key_result = result->generate_keys();
    ASSERT_TRUE(key_result.has_value())
        << key_result.error().message << " | " << key_result.error().detail;
    EXPECT_TRUE(result->has_keys());
}

TEST(FHEContext, EncryptDecryptRoundTrip) {
    auto ctx_result = FHEContext::create();
    ASSERT_TRUE(ctx_result.has_value()) << ctx_result.error().message;
    auto& ctx = *ctx_result;

    auto key_result = ctx.generate_keys();
    ASSERT_TRUE(key_result.has_value())
        << key_result.error().message << " | " << key_result.error().detail;

    // Create a test vector of integers
    const auto slots = ctx.slot_count();
    std::vector<int64_t> input(slots, 0);
    for (size_t i = 0; i < std::min(slots, size_t{100}); ++i) {
        input[i] = static_cast<int64_t>(i);
    }

    auto plaintext = ctx.context()->MakePackedPlaintext(input);
    auto ciphertext = ctx.context()->Encrypt(ctx.public_key(), plaintext);

    lbcrypto::Plaintext decrypted;
    ctx.context()->Decrypt(ctx.secret_key(), ciphertext, &decrypted);
    decrypted->SetLength(slots);

    auto output = decrypted->GetPackedValue();
    for (size_t i = 0; i < std::min(slots, size_t{100}); ++i) {
        EXPECT_EQ(output[i], static_cast<int64_t>(i)) << "at slot " << i;
    }
}

TEST(FHEContext, MultipleContextsIndependent) {
    auto ctx1 = FHEContext::create();
    auto ctx2 = FHEContext::create();
    ASSERT_TRUE(ctx1.has_value()) << ctx1.error().message;
    ASSERT_TRUE(ctx2.has_value()) << ctx2.error().message;

    auto k1 = ctx1->generate_keys();
    auto k2 = ctx2->generate_keys();
    ASSERT_TRUE(k1.has_value())
        << k1.error().message << " | " << k1.error().detail;
    ASSERT_TRUE(k2.has_value())
        << k2.error().message << " | " << k2.error().detail;

    // Both should have independent keys
    EXPECT_TRUE(ctx1->has_keys());
    EXPECT_TRUE(ctx2->has_keys());
    EXPECT_EQ(ctx1->slot_count(), ctx2->slot_count());
}

}  // namespace iris::test

#pragma GCC diagnostic pop

#endif  // IRIS_HAS_FHE
