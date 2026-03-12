#pragma once

#ifdef IRIS_HAS_FHE

#include <cstdint>
#include <span>
#include <vector>

#include <iris/core/error.hpp>
#include <iris/core/iris_code_packed.hpp>
#include <iris/crypto/fhe_context.hpp>

namespace iris {

/// An iris template encrypted under BFV homomorphic encryption.
///
/// Stores two ciphertexts: one for code bits and one for mask bits.
/// Each ciphertext packs 8192 bits as integers (0 or 1) in SIMD slots.
class EncryptedTemplate {
  public:
    /// Encrypt a PackedIrisCode.
    static Result<EncryptedTemplate> encrypt(
        const FHEContext& ctx, const PackedIrisCode& code);

    /// Decrypt back to a PackedIrisCode.
    Result<PackedIrisCode> decrypt(const FHEContext& ctx) const;

    /// Access encrypted code ciphertext (for server-side operations).
    [[nodiscard]] const lbcrypto::Ciphertext<lbcrypto::DCRTPoly>&
    code_ct() const noexcept;

    /// Access encrypted mask ciphertext (for server-side operations).
    [[nodiscard]] const lbcrypto::Ciphertext<lbcrypto::DCRTPoly>&
    mask_ct() const noexcept;

    /// Serialize to bytes (both ciphertexts).
    [[nodiscard]] Result<std::vector<uint8_t>> serialize() const;

    /// Deserialize from bytes.
    static Result<EncryptedTemplate> deserialize(
        const FHEContext& ctx, std::span<const uint8_t> data);

    /// Re-encrypt: decrypt with old context, encrypt with new context.
    static Result<EncryptedTemplate> re_encrypt(
        const FHEContext& old_ctx,
        const FHEContext& new_ctx,
        const EncryptedTemplate& old_encrypted);

  private:
    EncryptedTemplate() = default;

    lbcrypto::Ciphertext<lbcrypto::DCRTPoly> code_ct_;
    lbcrypto::Ciphertext<lbcrypto::DCRTPoly> mask_ct_;
};

}  // namespace iris

#endif  // IRIS_HAS_FHE
