#ifdef IRIS_HAS_FHE

#include <iris/crypto/key_store.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

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
#include "cryptocontext-ser.h"
#include "key/keypair.h"
#include "scheme/bfvrns/bfvrns-ser.h"

namespace iris {

namespace {

constexpr const char* kPublicKeyFile = "public_key.bin";
constexpr const char* kSecretKeyFile = "secret_key.bin";
constexpr const char* kMetaFile = "key_meta.json";

/// Serialize an OpenFHE object to a file.
template <typename T>
Result<void> serialize_to_file(const std::filesystem::path& path,
                                const T& obj) {
    try {
        std::ofstream os(path, std::ios::binary);
        if (!os) {
            return make_error(ErrorCode::IoFailed,
                              "Cannot open file for writing: " + path.string());
        }
        lbcrypto::Serial::Serialize(obj, os, lbcrypto::SerType::BINARY);
        return {};
    } catch (const std::exception& e) {
        return make_error(ErrorCode::IoFailed,
                          "Serialization failed: " + path.string(), e.what());
    }
}

/// Deserialize an OpenFHE object from a file.
template <typename T>
Result<void> deserialize_from_file(const std::filesystem::path& path,
                                    T& obj) {
    try {
        std::ifstream is(path, std::ios::binary);
        if (!is) {
            return make_error(ErrorCode::IoFailed,
                              "Cannot open file for reading: " + path.string());
        }
        lbcrypto::Serial::Deserialize(obj, is, lbcrypto::SerType::BINARY);
        return {};
    } catch (const std::exception& e) {
        return make_error(ErrorCode::IoFailed,
                          "Deserialization failed: " + path.string(), e.what());
    }
}

nlohmann::json metadata_to_json(const KeyMetadata& meta) {
    return nlohmann::json{
        {"key_id", meta.key_id},
        {"created_at", meta.created_at},
        {"expires_at", meta.expires_at},
        {"ring_dimension", meta.ring_dimension},
        {"plaintext_modulus", meta.plaintext_modulus},
        {"sha256_fingerprint", meta.sha256_fingerprint},
    };
}

Result<KeyMetadata> json_to_metadata(const nlohmann::json& j) {
    try {
        KeyMetadata meta;
        meta.key_id = j.at("key_id").get<std::string>();
        meta.created_at = j.at("created_at").get<std::string>();
        meta.expires_at = j.value("expires_at", std::string{});
        meta.ring_dimension = j.at("ring_dimension").get<uint32_t>();
        meta.plaintext_modulus = j.at("plaintext_modulus").get<uint32_t>();
        meta.sha256_fingerprint =
            j.value("sha256_fingerprint", std::string{});
        return meta;
    } catch (const std::exception& e) {
        return make_error(ErrorCode::IoFailed,
                          "Failed to parse key metadata", e.what());
    }
}

}  // namespace

Result<void> KeyStore::save(const std::filesystem::path& dir,
                             const FHEContext& ctx,
                             const KeyBundle& bundle) {
    try {
        std::filesystem::create_directories(dir);
    } catch (const std::exception& e) {
        return make_error(ErrorCode::IoFailed,
                          "Cannot create key directory", e.what());
    }

    // Save public key
    auto r = serialize_to_file(dir / kPublicKeyFile, bundle.public_key);
    if (!r) return r;

    // Save secret key
    r = serialize_to_file(dir / kSecretKeyFile, bundle.secret_key);
    if (!r) return r;

    // Restrict secret key permissions
    try {
        std::filesystem::permissions(dir / kSecretKeyFile,
            std::filesystem::perms::owner_read |
            std::filesystem::perms::owner_write,
            std::filesystem::perm_options::replace);
    } catch (const std::exception&) {
        // Best-effort: ignore if permission change fails (e.g. in Docker)
    }

    // Save metadata as JSON
    try {
        auto j = metadata_to_json(bundle.metadata);
        std::ofstream os(dir / kMetaFile);
        if (!os) {
            return make_error(ErrorCode::IoFailed,
                              "Cannot write metadata file");
        }
        os << j.dump(2) << '\n';
    } catch (const std::exception& e) {
        return make_error(ErrorCode::IoFailed,
                          "Metadata write failed", e.what());
    }

    return {};
}

Result<KeyBundle> KeyStore::load(const std::filesystem::path& dir,
                                  const FHEContext& ctx) {
    if (!std::filesystem::exists(dir)) {
        return make_error(ErrorCode::IoFailed,
                          "Key directory does not exist: " + dir.string());
    }

    KeyBundle bundle;

    // Load metadata
    {
        auto meta_path = dir / kMetaFile;
        std::ifstream is(meta_path);
        if (!is) {
            return make_error(ErrorCode::IoFailed,
                              "Cannot read metadata: " + meta_path.string());
        }
        try {
            auto j = nlohmann::json::parse(is);
            auto meta = json_to_metadata(j);
            if (!meta) return std::unexpected(meta.error());
            bundle.metadata = std::move(*meta);
        } catch (const std::exception& e) {
            return make_error(ErrorCode::IoFailed,
                              "Metadata parse failed", e.what());
        }
    }

    // Load public key
    auto r = deserialize_from_file(dir / kPublicKeyFile, bundle.public_key);
    if (!r) return std::unexpected(r.error());

    // Load secret key
    r = deserialize_from_file(dir / kSecretKeyFile, bundle.secret_key);
    if (!r) return std::unexpected(r.error());

    return bundle;
}

Result<KeyBundle> KeyStore::load_public_only(
    const std::filesystem::path& dir,
    const FHEContext& ctx) {
    if (!std::filesystem::exists(dir)) {
        return make_error(ErrorCode::IoFailed,
                          "Key directory does not exist: " + dir.string());
    }

    KeyBundle bundle;

    // Load metadata
    {
        auto meta_path = dir / kMetaFile;
        std::ifstream is(meta_path);
        if (!is) {
            return make_error(ErrorCode::IoFailed,
                              "Cannot read metadata: " + meta_path.string());
        }
        try {
            auto j = nlohmann::json::parse(is);
            auto meta = json_to_metadata(j);
            if (!meta) return std::unexpected(meta.error());
            bundle.metadata = std::move(*meta);
        } catch (const std::exception& e) {
            return make_error(ErrorCode::IoFailed,
                              "Metadata parse failed", e.what());
        }
    }

    // Load only public key
    auto r = deserialize_from_file(dir / kPublicKeyFile, bundle.public_key);
    if (!r) return std::unexpected(r.error());

    // secret_key remains null

    return bundle;
}

}  // namespace iris

#pragma GCC diagnostic pop

#endif  // IRIS_HAS_FHE
