#ifdef IRIS_HAS_FHE

#include <iris/crypto/encrypted_matcher.hpp>

#include <cstdint>
#include <vector>

// Suppress warnings from OpenFHE template instantiations (GCC 13 / cereal).
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

namespace iris {

Result<EncryptedResult> EncryptedMatcher::match_encrypted(
    const FHEContext& ctx,
    const EncryptedTemplate& probe,
    const EncryptedTemplate& gallery) {
    try {
        const auto& cc = ctx.context();
        const auto batch_size = static_cast<uint32_t>(ctx.slot_count());

        // XOR via (a - b)^2 for binary values
        auto ct_diff = cc->EvalSub(probe.code_ct(), gallery.code_ct());
        auto ct_xor = cc->EvalMult(ct_diff, ct_diff);

        // AND masks
        auto ct_mask = cc->EvalMult(probe.mask_ct(), gallery.mask_ct());

        // Masked XOR (numerator before sum)
        auto ct_masked = cc->EvalMult(ct_xor, ct_mask);

        // Sum across all slots
        auto ct_numerator = cc->EvalSum(ct_masked, batch_size);
        auto ct_denominator = cc->EvalSum(ct_mask, batch_size);

        return EncryptedResult{std::move(ct_numerator),
                               std::move(ct_denominator)};
    } catch (const std::exception& e) {
        return make_error(ErrorCode::CryptoFailed,
                          "Encrypted match failed", e.what());
    }
}

Result<double> EncryptedMatcher::decrypt_result(
    const FHEContext& ctx,
    const EncryptedResult& result) {
    if (!ctx.has_keys()) {
        return make_error(ErrorCode::CryptoFailed,
                          "Cannot decrypt without keys");
    }

    try {
        lbcrypto::Plaintext pt_num;
        lbcrypto::Plaintext pt_den;

        ctx.context()->Decrypt(ctx.secret_key(), result.ct_numerator, &pt_num);
        ctx.context()->Decrypt(ctx.secret_key(), result.ct_denominator, &pt_den);

        pt_num->SetLength(1);
        pt_den->SetLength(1);

        auto num = pt_num->GetPackedValue()[0];
        auto den = pt_den->GetPackedValue()[0];

        if (den == 0) {
            return 1.0;  // No valid bits → maximum distance
        }

        return static_cast<double>(num) / static_cast<double>(den);
    } catch (const std::exception& e) {
        return make_error(ErrorCode::CryptoFailed,
                          "Result decryption failed", e.what());
    }
}

Result<double> EncryptedMatcher::match(
    const FHEContext& ctx,
    const PackedIrisCode& probe,
    const PackedIrisCode& gallery) {
    auto enc_probe = EncryptedTemplate::encrypt(ctx, probe);
    if (!enc_probe) return std::unexpected(enc_probe.error());

    auto enc_gallery = EncryptedTemplate::encrypt(ctx, gallery);
    if (!enc_gallery) return std::unexpected(enc_gallery.error());

    auto result = match_encrypted(ctx, *enc_probe, *enc_gallery);
    if (!result) return std::unexpected(result.error());

    return decrypt_result(ctx, *result);
}

Result<EncryptedMatchResult> EncryptedMatcher::match_with_rotation(
    const FHEContext& ctx,
    const PackedIrisCode& probe,
    const PackedIrisCode& gallery,
    int rotation_shift) {
    // Encrypt gallery once
    auto enc_gallery = EncryptedTemplate::encrypt(ctx, gallery);
    if (!enc_gallery) return std::unexpected(enc_gallery.error());

    EncryptedMatchResult best{.distance = 1.0, .best_rotation = 0};

    for (int abs_shift = 0; abs_shift <= rotation_shift; ++abs_shift) {
        for (int sign : {1, -1}) {
            if (abs_shift == 0 && sign == -1) continue;
            const int shift = abs_shift * sign;

            // Rotate probe in plaintext
            auto rotated_probe = probe.rotate(shift);

            // Encrypt rotated probe
            auto enc_probe = EncryptedTemplate::encrypt(ctx, rotated_probe);
            if (!enc_probe) return std::unexpected(enc_probe.error());

            // Encrypted match
            auto result = match_encrypted(ctx, *enc_probe, *enc_gallery);
            if (!result) return std::unexpected(result.error());

            // Decrypt
            auto hd = decrypt_result(ctx, *result);
            if (!hd) return std::unexpected(hd.error());

            if (*hd < best.distance) {
                best.distance = *hd;
                best.best_rotation = shift;
            }
        }
    }

    return best;
}

}  // namespace iris

#pragma GCC diagnostic pop

#endif  // IRIS_HAS_FHE
