#ifdef IRIS_HAS_FHE

#include <iris/crypto/encrypted_template.hpp>

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

// Suppress warnings from OpenFHE headers and template instantiations.
// Must cover the entire file because serialization calls instantiate cereal
// templates that trigger -Wnull-dereference in GCC.
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
#include "ciphertext-ser.h"
#include "cryptocontext-ser.h"
#include "scheme/bfvrns/bfvrns-ser.h"

namespace iris {

namespace {

/// Pack bits from a uint64_t array into a vector of int64_t (0 or 1) for
/// SIMD slot packing.
std::vector<int64_t> bits_to_slots(
    const std::array<uint64_t, PackedIrisCode::kNumWords>& bits,
    size_t slot_count) {
    std::vector<int64_t> slots(slot_count, 0);
    for (size_t i = 0; i < PackedIrisCode::kTotalBits && i < slot_count; ++i) {
        size_t word = i / 64;
        size_t bit = i % 64;
        slots[i] = static_cast<int64_t>((bits[word] >> bit) & 1ULL);
    }
    return slots;
}

/// Unpack SIMD slot values back into a uint64_t bit array.
void slots_to_bits(
    const std::vector<int64_t>& slots,
    std::array<uint64_t, PackedIrisCode::kNumWords>& bits) {
    bits = {};
    for (size_t i = 0; i < PackedIrisCode::kTotalBits && i < slots.size();
         ++i) {
        if (slots[i] != 0) {
            size_t word = i / 64;
            size_t bit = i % 64;
            bits[word] |= (1ULL << bit);
        }
    }
}

}  // namespace

Result<EncryptedTemplate> EncryptedTemplate::encrypt(
    const FHEContext& ctx, const PackedIrisCode& code) {
    if (!ctx.has_keys()) {
        return make_error(ErrorCode::CryptoFailed,
                          "Cannot encrypt without keys");
    }

    EncryptedTemplate et;
    try {
        auto code_slots = bits_to_slots(code.code_bits, ctx.slot_count());
        auto mask_slots = bits_to_slots(code.mask_bits, ctx.slot_count());

        auto code_pt = ctx.context()->MakePackedPlaintext(code_slots);
        auto mask_pt = ctx.context()->MakePackedPlaintext(mask_slots);

        et.code_ct_ = ctx.context()->Encrypt(ctx.public_key(), code_pt);
        et.mask_ct_ = ctx.context()->Encrypt(ctx.public_key(), mask_pt);
    } catch (const std::exception& e) {
        return make_error(ErrorCode::CryptoFailed,
                          "Template encryption failed", e.what());
    }

    return et;
}

Result<PackedIrisCode> EncryptedTemplate::decrypt(
    const FHEContext& ctx) const {
    if (!ctx.has_keys()) {
        return make_error(ErrorCode::CryptoFailed,
                          "Cannot decrypt without keys");
    }

    PackedIrisCode code;
    try {
        lbcrypto::Plaintext code_pt;
        lbcrypto::Plaintext mask_pt;
        ctx.context()->Decrypt(ctx.secret_key(), code_ct_, &code_pt);
        ctx.context()->Decrypt(ctx.secret_key(), mask_ct_, &mask_pt);

        code_pt->SetLength(ctx.slot_count());
        mask_pt->SetLength(ctx.slot_count());

        slots_to_bits(code_pt->GetPackedValue(), code.code_bits);
        slots_to_bits(mask_pt->GetPackedValue(), code.mask_bits);
    } catch (const std::exception& e) {
        return make_error(ErrorCode::CryptoFailed,
                          "Template decryption failed", e.what());
    }

    return code;
}

const lbcrypto::Ciphertext<lbcrypto::DCRTPoly>&
EncryptedTemplate::code_ct() const noexcept {
    return code_ct_;
}

const lbcrypto::Ciphertext<lbcrypto::DCRTPoly>&
EncryptedTemplate::mask_ct() const noexcept {
    return mask_ct_;
}

Result<std::vector<uint8_t>> EncryptedTemplate::serialize() const {
    try {
        std::ostringstream os(std::ios::binary);

        // Write code ciphertext length + data, then mask ciphertext
        std::ostringstream code_os(std::ios::binary);
        lbcrypto::Serial::Serialize(code_ct_, code_os, lbcrypto::SerType::BINARY);
        std::string code_data = code_os.str();

        std::ostringstream mask_os(std::ios::binary);
        lbcrypto::Serial::Serialize(mask_ct_, mask_os, lbcrypto::SerType::BINARY);
        std::string mask_data = mask_os.str();

        // Format: [code_size(8)][code_data][mask_size(8)][mask_data]
        auto code_size = static_cast<uint64_t>(code_data.size());
        auto mask_size = static_cast<uint64_t>(mask_data.size());

        os.write(reinterpret_cast<const char*>(&code_size), sizeof(code_size));
        os.write(code_data.data(), static_cast<std::streamsize>(code_data.size()));
        os.write(reinterpret_cast<const char*>(&mask_size), sizeof(mask_size));
        os.write(mask_data.data(), static_cast<std::streamsize>(mask_data.size()));

        std::string result = os.str();
        return std::vector<uint8_t>(result.begin(), result.end());
    } catch (const std::exception& e) {
        return make_error(ErrorCode::CryptoFailed,
                          "Template serialization failed", e.what());
    }
}

Result<EncryptedTemplate> EncryptedTemplate::re_encrypt(
    const FHEContext& old_ctx,
    const FHEContext& new_ctx,
    const EncryptedTemplate& old_encrypted) {
    auto plaintext = old_encrypted.decrypt(old_ctx);
    if (!plaintext) {
        return std::unexpected(plaintext.error());
    }
    return encrypt(new_ctx, *plaintext);
}

Result<EncryptedTemplate> EncryptedTemplate::deserialize(
    const FHEContext& ctx, std::span<const uint8_t> data) {
    EncryptedTemplate et;
    try {
        if (data.size() < 16) {
            return make_error(ErrorCode::CryptoFailed,
                              "Serialized data too small");
        }

        size_t offset = 0;

        uint64_t code_size = 0;
        std::memcpy(&code_size, data.data() + offset, sizeof(code_size));
        offset += sizeof(code_size);

        if (offset + code_size + sizeof(uint64_t) > data.size()) {
            return make_error(ErrorCode::CryptoFailed,
                              "Truncated serialized data");
        }

        std::istringstream code_is(
            std::string(reinterpret_cast<const char*>(data.data() + offset),
                        static_cast<size_t>(code_size)),
            std::ios::binary);
        lbcrypto::Serial::Deserialize(et.code_ct_, code_is, lbcrypto::SerType::BINARY);
        offset += static_cast<size_t>(code_size);

        uint64_t mask_size = 0;
        std::memcpy(&mask_size, data.data() + offset, sizeof(mask_size));
        offset += sizeof(mask_size);

        if (offset + static_cast<size_t>(mask_size) > data.size()) {
            return make_error(ErrorCode::CryptoFailed,
                              "Truncated serialized mask data");
        }

        std::istringstream mask_is(
            std::string(reinterpret_cast<const char*>(data.data() + offset),
                        static_cast<size_t>(mask_size)),
            std::ios::binary);
        lbcrypto::Serial::Deserialize(et.mask_ct_, mask_is, lbcrypto::SerType::BINARY);

    } catch (const std::exception& e) {
        return make_error(ErrorCode::CryptoFailed,
                          "Template deserialization failed", e.what());
    }

    return et;
}

}  // namespace iris

#pragma GCC diagnostic pop

#endif  // IRIS_HAS_FHE
