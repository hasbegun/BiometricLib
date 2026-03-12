#pragma once

#ifdef IRIS_HAS_FHE

#include <chrono>
#include <optional>
#include <string>

#include <iris/core/error.hpp>
#include <iris/crypto/fhe_context.hpp>

namespace iris {

/// Metadata about a generated key bundle.
struct KeyMetadata {
    std::string key_id;
    std::string created_at;              // ISO 8601
    std::string expires_at;              // ISO 8601 or empty (no expiry)
    uint32_t ring_dimension = 0;
    uint32_t plaintext_modulus = 0;
    std::string sha256_fingerprint;      // hex digest of serialized public key
};

/// A complete key bundle: keys + metadata.
struct KeyBundle {
    lbcrypto::PublicKey<lbcrypto::DCRTPoly> public_key;
    lbcrypto::PrivateKey<lbcrypto::DCRTPoly> secret_key;
    KeyMetadata metadata;
};

/// Key generation and validation utilities.
class KeyManager {
  public:
    /// Generate a new key bundle from the context.
    /// Optionally set a TTL for expiry.
    static Result<KeyBundle> generate(
        const FHEContext& ctx,
        std::optional<std::chrono::seconds> ttl = {});

    /// Validate that a key bundle is consistent with the context.
    static Result<void> validate(const FHEContext& ctx,
                                 const KeyBundle& bundle);

    /// Check if key metadata indicates expiry.
    static bool is_expired(const KeyMetadata& meta);
};

}  // namespace iris

#endif  // IRIS_HAS_FHE
