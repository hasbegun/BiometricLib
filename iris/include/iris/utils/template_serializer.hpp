#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <iris/core/error.hpp>
#include <iris/core/iris_code_packed.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Serialization for iris templates in native binary and Python-compatible formats.
class TemplateSerializer {
  public:
    /// Native binary format (2056 bytes per PackedIrisCode).
    /// Layout: "IRTB" magic (4) + version (1) + flags (1) + reserved (2)
    ///       + code_bits (1024) + mask_bits (1024).
    static std::vector<uint8_t> to_binary(const PackedIrisCode& code);
    static Result<PackedIrisCode> from_binary(std::span<const uint8_t> data);

    /// Python-compatible JSON format (base64 + packbits).
    /// Matches open-iris `IrisTemplate.serialize()` output.
    /// JSON keys: "iris_codes", "mask_codes", "iris_code_version".
    static std::string to_python_format(const IrisTemplate& tmpl);
    static Result<IrisTemplate> from_python_format(std::string_view json);

    /// Binary format constants.
    static constexpr size_t kBinarySize = 2056;
    static constexpr std::array<char, 4> kMagic = {'I', 'R', 'T', 'B'};
    static constexpr uint8_t kVersion = 1;
};

}  // namespace iris
