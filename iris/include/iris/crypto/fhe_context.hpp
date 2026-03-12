#pragma once

#ifdef IRIS_HAS_FHE

#include <cstddef>
#include <cstdint>

#include <iris/core/error.hpp>

// Suppress warnings from OpenFHE headers
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
#include "openfhe.h"
#pragma GCC diagnostic pop

namespace iris {

/// Parameters for BFV homomorphic encryption context.
struct FHEParams {
    uint32_t plaintext_modulus = 65537;
    uint32_t mult_depth = 2;
    lbcrypto::SecurityLevel security_level = lbcrypto::HEStd_128_classic;
    uint32_t ring_dimension = 16384;  // → 8192 SIMD slots for BFV batching
};

/// Wrapper around OpenFHE BFV crypto context.
///
/// Provides a simplified interface for iris template encryption:
/// - Context creation with validated parameters
/// - Key generation (public, secret, evaluation keys)
/// - Access to underlying OpenFHE context for operations
class FHEContext {
  public:
    /// Create an FHE context with the given parameters.
    static Result<FHEContext> create(const FHEParams& params);

    /// Create with default parameters (plaintext_modulus=65537, mult_depth=2,
    /// ring_dim=16384).
    static Result<FHEContext> create();

    /// Number of SIMD slots available for batching (ring_dim / 2).
    [[nodiscard]] size_t slot_count() const noexcept;

    /// Generate all keys (public, secret, eval mult, eval sum, eval rotate).
    Result<void> generate_keys();

    /// Whether keys have been generated.
    [[nodiscard]] bool has_keys() const noexcept;

    /// Access the underlying crypto context.
    [[nodiscard]] const lbcrypto::CryptoContext<lbcrypto::DCRTPoly>&
    context() const noexcept;

    /// Access keys (require generate_keys() to have been called).
    [[nodiscard]] const lbcrypto::PublicKey<lbcrypto::DCRTPoly>&
    public_key() const;

    [[nodiscard]] const lbcrypto::PrivateKey<lbcrypto::DCRTPoly>&
    secret_key() const;

  private:
    FHEContext() = default;

    lbcrypto::CryptoContext<lbcrypto::DCRTPoly> cc_;
    lbcrypto::KeyPair<lbcrypto::DCRTPoly> key_pair_;
    size_t slot_count_ = 0;
    bool has_keys_ = false;
};

}  // namespace iris

#endif  // IRIS_HAS_FHE
