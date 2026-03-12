#pragma once

#ifdef IRIS_HAS_FHE

#include <filesystem>

#include <iris/core/error.hpp>
#include <iris/crypto/fhe_context.hpp>
#include <iris/crypto/key_manager.hpp>

namespace iris {

/// Persistent key storage on the filesystem.
///
/// Directory layout:
///   public_key.bin     — serialized public key
///   secret_key.bin     — serialized secret key (0600 permissions)
///   eval_mult_key.bin  — EvalMultKey
///   eval_sum_key.bin   — EvalSumKey (unused for now, kept for future)
///   key_meta.json      — KeyMetadata as JSON
class KeyStore {
  public:
    /// Save a key bundle to disk.
    static Result<void> save(const std::filesystem::path& dir,
                             const FHEContext& ctx,
                             const KeyBundle& bundle);

    /// Load a full key bundle (public + secret + metadata).
    static Result<KeyBundle> load(const std::filesystem::path& dir,
                                  const FHEContext& ctx);

    /// Load only public key and metadata (no secret key).
    static Result<KeyBundle> load_public_only(
        const std::filesystem::path& dir,
        const FHEContext& ctx);
};

}  // namespace iris

#endif  // IRIS_HAS_FHE
