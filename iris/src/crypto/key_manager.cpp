#ifdef IRIS_HAS_FHE

#include <iris/crypto/key_manager.hpp>

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

// Suppress warnings from OpenFHE template instantiations.
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
#include "key/keypair.h"
#include "cryptocontext-ser.h"
#include "scheme/bfvrns/bfvrns-ser.h"

namespace iris {

namespace {

/// Generate a UUID-like identifier.
std::string generate_key_id() {
    std::random_device rd;
    std::mt19937_64 rng(rd());
    std::uniform_int_distribution<uint64_t> dist;

    auto a = dist(rng);
    auto b = dist(rng);

    std::ostringstream os;
    os << std::hex << std::setfill('0');
    os << std::setw(8) << (a >> 32) << '-';
    os << std::setw(4) << ((a >> 16) & 0xFFFF) << '-';
    os << std::setw(4) << (a & 0xFFFF) << '-';
    os << std::setw(4) << (b >> 48) << '-';
    os << std::setw(12) << (b & 0xFFFFFFFFFFFFULL);
    return os.str();
}

/// Current time as ISO 8601 string.
std::string now_iso8601() {
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    gmtime_r(&tt, &tm);
    std::ostringstream os;
    os << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return os.str();
}

/// Compute a simple fingerprint from serialized public key.
/// Uses a basic hash (not SHA-256, no crypto dependency).
std::string compute_fingerprint(
    const lbcrypto::PublicKey<lbcrypto::DCRTPoly>& pk) {
    std::ostringstream os(std::ios::binary);
    lbcrypto::Serial::Serialize(pk, os, lbcrypto::SerType::BINARY);
    std::string data = os.str();

    // FNV-1a 64-bit as a fast fingerprint
    uint64_t hash = 14695981039346656037ULL;
    for (unsigned char c : data) {
        hash ^= c;
        hash *= 1099511628211ULL;
    }

    std::ostringstream hex_os;
    hex_os << std::hex << std::setfill('0') << std::setw(16) << hash;
    return hex_os.str();
}

}  // namespace

Result<KeyBundle> KeyManager::generate(
    const FHEContext& ctx,
    std::optional<std::chrono::seconds> ttl) {
    if (!ctx.has_keys()) {
        return make_error(ErrorCode::CryptoFailed,
                          "Context must have keys before generating bundle");
    }

    KeyBundle bundle;
    bundle.public_key = ctx.public_key();
    bundle.secret_key = ctx.secret_key();

    bundle.metadata.key_id = generate_key_id();
    bundle.metadata.created_at = now_iso8601();

    if (ttl) {
        auto expiry = std::chrono::system_clock::now() + *ttl;
        auto tt = std::chrono::system_clock::to_time_t(expiry);
        std::tm tm{};
        gmtime_r(&tt, &tm);
        std::ostringstream os;
        os << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
        bundle.metadata.expires_at = os.str();
    }

    bundle.metadata.ring_dimension =
        static_cast<uint32_t>(ctx.context()->GetRingDimension());
    bundle.metadata.plaintext_modulus =
        static_cast<uint32_t>(ctx.context()->GetCryptoParameters()
                                  ->GetPlaintextModulus());

    try {
        bundle.metadata.sha256_fingerprint =
            compute_fingerprint(bundle.public_key);
    } catch (const std::exception& e) {
        return make_error(ErrorCode::CryptoFailed,
                          "Fingerprint computation failed", e.what());
    }

    return bundle;
}

Result<void> KeyManager::validate(const FHEContext& ctx,
                                   const KeyBundle& bundle) {
    auto ring_dim = static_cast<uint32_t>(ctx.context()->GetRingDimension());
    if (bundle.metadata.ring_dimension != ring_dim) {
        return make_error(ErrorCode::CryptoFailed,
                          "Key ring dimension mismatch");
    }

    auto pt_mod = static_cast<uint32_t>(
        ctx.context()->GetCryptoParameters()->GetPlaintextModulus());
    if (bundle.metadata.plaintext_modulus != pt_mod) {
        return make_error(ErrorCode::CryptoFailed,
                          "Key plaintext modulus mismatch");
    }

    if (is_expired(bundle.metadata)) {
        return make_error(ErrorCode::CryptoFailed, "Key has expired");
    }

    return {};
}

bool KeyManager::is_expired(const KeyMetadata& meta) {
    if (meta.expires_at.empty()) return false;

    // Parse ISO 8601 timestamp
    std::tm tm{};
    std::istringstream is(meta.expires_at);
    is >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    if (is.fail()) return false;  // unparseable = not expired

    auto expiry = std::chrono::system_clock::from_time_t(timegm(&tm));
    return std::chrono::system_clock::now() > expiry;
}

}  // namespace iris

#pragma GCC diagnostic pop

#endif  // IRIS_HAS_FHE
