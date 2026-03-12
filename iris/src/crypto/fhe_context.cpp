#ifdef IRIS_HAS_FHE

#include <iris/crypto/fhe_context.hpp>

#include <vector>

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
#include "scheme/bfvrns/gen-cryptocontext-bfvrns.h"
#include "gen-cryptocontext.h"
#pragma GCC diagnostic pop

namespace iris {

Result<FHEContext> FHEContext::create(const FHEParams& params) {
    FHEContext ctx;

    try {
        lbcrypto::CCParams<lbcrypto::CryptoContextBFVRNS> cc_params;
        cc_params.SetPlaintextModulus(params.plaintext_modulus);
        cc_params.SetMultiplicativeDepth(params.mult_depth);
        cc_params.SetSecurityLevel(params.security_level);
        cc_params.SetRingDim(params.ring_dimension);

        ctx.cc_ = lbcrypto::GenCryptoContext(cc_params);
        if (!ctx.cc_) {
            return make_error(ErrorCode::CryptoFailed,
                              "Failed to generate BFV crypto context");
        }

        ctx.cc_->Enable(lbcrypto::PKE);
        ctx.cc_->Enable(lbcrypto::KEYSWITCH);
        ctx.cc_->Enable(lbcrypto::LEVELEDSHE);
        ctx.cc_->Enable(lbcrypto::ADVANCEDSHE);

        // slot_count = ring_dim / 2 for BFV batching
        ctx.slot_count_ = static_cast<size_t>(ctx.cc_->GetRingDimension()) / 2;

    } catch (const std::exception& e) {
        return make_error(ErrorCode::CryptoFailed,
                          "FHEContext creation failed", e.what());
    }

    return ctx;
}

Result<FHEContext> FHEContext::create() {
    FHEParams params;
    return create(params);
}

size_t FHEContext::slot_count() const noexcept {
    return slot_count_;
}

Result<void> FHEContext::generate_keys() {
    try {
        key_pair_ = cc_->KeyGen();
        if (!key_pair_.publicKey || !key_pair_.secretKey) {
            return make_error(ErrorCode::CryptoFailed,
                              "Key generation produced null keys");
        }

        cc_->EvalMultKeyGen(key_pair_.secretKey);
        cc_->EvalSumKeyGen(key_pair_.secretKey);

        // Generate rotation keys for barrel shifts (needed for template
        // rotation under encryption). Power-of-2 rotations cover arbitrary
        // shifts via composition.
        std::vector<int32_t> rotation_indices;
        const auto slots = static_cast<int32_t>(slot_count_);
        for (int32_t r = 1; r < slots; r *= 2) {
            rotation_indices.push_back(r);
            rotation_indices.push_back(-r);
        }
        cc_->EvalRotateKeyGen(key_pair_.secretKey, rotation_indices);

        has_keys_ = true;
    } catch (const std::exception& e) {
        return make_error(ErrorCode::CryptoFailed,
                          "Key generation failed", e.what());
    }

    return {};
}

bool FHEContext::has_keys() const noexcept {
    return has_keys_;
}

const lbcrypto::CryptoContext<lbcrypto::DCRTPoly>&
FHEContext::context() const noexcept {
    return cc_;
}

const lbcrypto::PublicKey<lbcrypto::DCRTPoly>&
FHEContext::public_key() const {
    return key_pair_.publicKey;
}

const lbcrypto::PrivateKey<lbcrypto::DCRTPoly>&
FHEContext::secret_key() const {
    return key_pair_.secretKey;
}

}  // namespace iris

#endif  // IRIS_HAS_FHE
