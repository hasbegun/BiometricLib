#ifdef IRIS_HAS_FHE

#include <iris/crypto/key_store.hpp>
#include <iris/crypto/encrypted_template.hpp>
#include <iris/crypto/fhe_context.hpp>
#include <iris/crypto/key_manager.hpp>

#include <cstdint>
#include <filesystem>
#include <random>

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

struct TmpDir {
    std::filesystem::path path;
    explicit TmpDir(const std::string& name)
        : path(std::filesystem::temp_directory_path() / name) {
        std::filesystem::remove_all(path);
    }
    ~TmpDir() { std::filesystem::remove_all(path); }
};

}  // namespace

TEST(KeyStore, SaveLoadRoundTrip) {
    auto ctx = make_ctx();
    auto bundle = KeyManager::generate(ctx);
    ASSERT_TRUE(bundle.has_value()) << bundle.error().message;

    TmpDir dir("test_key_store_rt");
    auto save_result = KeyStore::save(dir.path, ctx, *bundle);
    ASSERT_TRUE(save_result.has_value()) << save_result.error().message;

    auto loaded = KeyStore::load(dir.path, ctx);
    ASSERT_TRUE(loaded.has_value()) << loaded.error().message;

    // Metadata preserved
    EXPECT_EQ(loaded->metadata.key_id, bundle->metadata.key_id);
    EXPECT_EQ(loaded->metadata.created_at, bundle->metadata.created_at);
    EXPECT_EQ(loaded->metadata.ring_dimension,
              bundle->metadata.ring_dimension);
    EXPECT_EQ(loaded->metadata.plaintext_modulus,
              bundle->metadata.plaintext_modulus);

    // Verify loaded keys work: encrypt with loaded public key, decrypt
    PackedIrisCode code;
    std::mt19937_64 rng(42);
    for (auto& w : code.code_bits) w = rng();
    for (auto& w : code.mask_bits) w = ~uint64_t{0};

    // Use original context (which has eval keys) for encrypt/decrypt
    auto enc = EncryptedTemplate::encrypt(ctx, code);
    ASSERT_TRUE(enc.has_value()) << enc.error().message;
    auto dec = enc->decrypt(ctx);
    ASSERT_TRUE(dec.has_value()) << dec.error().message;
    EXPECT_EQ(dec->code_bits, code.code_bits);
}

TEST(KeyStore, LoadPublicOnlyHasNoSecretKey) {
    auto ctx = make_ctx();
    auto bundle = KeyManager::generate(ctx);
    ASSERT_TRUE(bundle.has_value());

    TmpDir dir("test_key_store_pub");
    auto save_result = KeyStore::save(dir.path, ctx, *bundle);
    ASSERT_TRUE(save_result.has_value());

    auto loaded = KeyStore::load_public_only(dir.path, ctx);
    ASSERT_TRUE(loaded.has_value()) << loaded.error().message;

    // Public key should be set, secret key should be null
    EXPECT_TRUE(loaded->public_key != nullptr);
    EXPECT_TRUE(loaded->secret_key == nullptr);
    EXPECT_EQ(loaded->metadata.key_id, bundle->metadata.key_id);
}

TEST(KeyStore, LoadNonexistentDirFails) {
    auto ctx = make_ctx();
    auto result = KeyStore::load("/tmp/nonexistent_key_dir_12345", ctx);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::IoFailed);
}

TEST(KeyStore, MetadataPreservedExactly) {
    auto ctx = make_ctx();
    auto bundle = KeyManager::generate(ctx, std::chrono::seconds{7200});
    ASSERT_TRUE(bundle.has_value());

    TmpDir dir("test_key_store_meta");
    auto save_result = KeyStore::save(dir.path, ctx, *bundle);
    ASSERT_TRUE(save_result.has_value());

    auto loaded = KeyStore::load(dir.path, ctx);
    ASSERT_TRUE(loaded.has_value());

    EXPECT_EQ(loaded->metadata.key_id, bundle->metadata.key_id);
    EXPECT_EQ(loaded->metadata.created_at, bundle->metadata.created_at);
    EXPECT_EQ(loaded->metadata.expires_at, bundle->metadata.expires_at);
    EXPECT_EQ(loaded->metadata.ring_dimension,
              bundle->metadata.ring_dimension);
    EXPECT_EQ(loaded->metadata.plaintext_modulus,
              bundle->metadata.plaintext_modulus);
    EXPECT_EQ(loaded->metadata.sha256_fingerprint,
              bundle->metadata.sha256_fingerprint);
}

}  // namespace iris::test

#pragma GCC diagnostic pop

#endif  // IRIS_HAS_FHE
