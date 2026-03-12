#pragma once

#ifdef IRIS_HAS_FHE

#include <cstdint>

#include <iris/core/error.hpp>
#include <iris/core/iris_code_packed.hpp>
#include <iris/crypto/encrypted_template.hpp>
#include <iris/crypto/fhe_context.hpp>

namespace iris {

/// Result of a single encrypted match (before decryption).
struct EncryptedResult {
    lbcrypto::Ciphertext<lbcrypto::DCRTPoly> ct_numerator;
    lbcrypto::Ciphertext<lbcrypto::DCRTPoly> ct_denominator;
};

/// Result of a match with rotation.
struct EncryptedMatchResult {
    double distance = 1.0;
    int best_rotation = 0;
};

/// Hamming distance matcher operating on FHE-encrypted iris templates.
///
/// HD computation under encryption (for binary values 0/1):
///   ct_diff = EvalSub(probe_code, gallery_code)
///   ct_xor  = EvalMult(ct_diff, ct_diff)      // (a-b)^2 = XOR
///   ct_mask = EvalMult(probe_mask, gallery_mask) // AND masks
///   ct_num  = EvalSum(EvalMult(ct_xor, ct_mask)) // masked diff count
///   ct_den  = EvalSum(ct_mask)                   // valid bit count
///   HD = decrypt(ct_num) / decrypt(ct_den)
class EncryptedMatcher {
  public:
    /// Server-side: compute encrypted HD numerator and denominator.
    static Result<EncryptedResult> match_encrypted(
        const FHEContext& ctx,
        const EncryptedTemplate& probe,
        const EncryptedTemplate& gallery);

    /// Client-side: decrypt result and compute final HD.
    static Result<double> decrypt_result(
        const FHEContext& ctx,
        const EncryptedResult& result);

    /// Convenience: encrypt + match + decrypt in one step.
    static Result<double> match(
        const FHEContext& ctx,
        const PackedIrisCode& probe,
        const PackedIrisCode& gallery);

    /// Match with rotation alignment (probe-side rotation).
    /// For each shift in [-rotation_shift, +rotation_shift]:
    ///   1. Rotate probe in plaintext
    ///   2. Encrypt rotated probe
    ///   3. Match encrypted against gallery
    ///   4. Decrypt result
    ///   5. Return minimum HD across all shifts
    static Result<EncryptedMatchResult> match_with_rotation(
        const FHEContext& ctx,
        const PackedIrisCode& probe,
        const PackedIrisCode& gallery,
        int rotation_shift = 15);
};

}  // namespace iris

#endif  // IRIS_HAS_FHE
