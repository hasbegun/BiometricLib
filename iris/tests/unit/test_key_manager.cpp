#ifdef IRIS_HAS_FHE

#include <iris/crypto/key_manager.hpp>
#include <iris/crypto/fhe_context.hpp>

#include <chrono>

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
}  // namespace

TEST(KeyManager, GenerateSucceeds) {
    auto ctx = make_ctx();
    auto bundle = KeyManager::generate(ctx);
    ASSERT_TRUE(bundle.has_value()) << bundle.error().message;

    EXPECT_FALSE(bundle->metadata.key_id.empty());
    EXPECT_FALSE(bundle->metadata.created_at.empty());
    EXPECT_TRUE(bundle->metadata.expires_at.empty());  // no TTL
    EXPECT_EQ(bundle->metadata.ring_dimension, 16384u);
    EXPECT_EQ(bundle->metadata.plaintext_modulus, 65537u);
    EXPECT_FALSE(bundle->metadata.sha256_fingerprint.empty());
}

TEST(KeyManager, ValidateSucceeds) {
    auto ctx = make_ctx();
    auto bundle = KeyManager::generate(ctx);
    ASSERT_TRUE(bundle.has_value());

    auto result = KeyManager::validate(ctx, *bundle);
    EXPECT_TRUE(result.has_value()) << result.error().message;
}

TEST(KeyManager, ExpiredKeyDetected) {
    KeyMetadata meta;
    meta.expires_at = "2020-01-01T00:00:00Z";  // past date
    EXPECT_TRUE(KeyManager::is_expired(meta));
}

TEST(KeyManager, NonExpiredKeyNotDetected) {
    KeyMetadata meta;
    meta.expires_at = "2099-12-31T23:59:59Z";  // far future
    EXPECT_FALSE(KeyManager::is_expired(meta));
}

TEST(KeyManager, EmptyExpiryNotExpired) {
    KeyMetadata meta;
    meta.expires_at = "";  // no expiry
    EXPECT_FALSE(KeyManager::is_expired(meta));
}

TEST(KeyManager, GenerateWithTTL) {
    using namespace std::chrono_literals;
    auto ctx = make_ctx();
    auto bundle = KeyManager::generate(ctx, 3600s);
    ASSERT_TRUE(bundle.has_value());

    EXPECT_FALSE(bundle->metadata.expires_at.empty());
    EXPECT_FALSE(KeyManager::is_expired(bundle->metadata));
}

}  // namespace iris::test

#pragma GCC diagnostic pop

#endif  // IRIS_HAS_FHE
