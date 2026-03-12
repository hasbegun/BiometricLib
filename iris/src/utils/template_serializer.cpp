#include <iris/utils/template_serializer.hpp>

#include <iris/utils/base64.hpp>

#include <algorithm>
#include <cstring>

#include <nlohmann/json.hpp>

namespace iris {

// --- Binary format ---

std::vector<uint8_t> TemplateSerializer::to_binary(const PackedIrisCode& code) {
    std::vector<uint8_t> buf(kBinarySize);
    auto* p = buf.data();

    // Magic
    std::memcpy(p, kMagic.data(), 4);
    p += 4;

    // Version
    *p++ = kVersion;

    // Flags: bit 0 = has_mask (always true for now)
    *p++ = 0x01;

    // Reserved
    *p++ = 0;
    *p++ = 0;

    // code_bits: 128 x uint64_t in little-endian
    for (size_t i = 0; i < PackedIrisCode::kNumWords; ++i) {
        uint64_t w = code.code_bits[i];
        for (int b = 0; b < 8; ++b) {
            *p++ = static_cast<uint8_t>(w & 0xFF);
            w >>= 8;
        }
    }

    // mask_bits: 128 x uint64_t in little-endian
    for (size_t i = 0; i < PackedIrisCode::kNumWords; ++i) {
        uint64_t w = code.mask_bits[i];
        for (int b = 0; b < 8; ++b) {
            *p++ = static_cast<uint8_t>(w & 0xFF);
            w >>= 8;
        }
    }

    return buf;
}

Result<PackedIrisCode> TemplateSerializer::from_binary(std::span<const uint8_t> data) {
    if (data.size() < kBinarySize) {
        return make_error(ErrorCode::IoFailed, "Binary template too short",
                          "Expected " + std::to_string(kBinarySize) + " bytes, got " +
                              std::to_string(data.size()));
    }

    const auto* p = data.data();

    // Check magic
    if (std::memcmp(p, kMagic.data(), 4) != 0) {
        return make_error(ErrorCode::IoFailed, "Invalid template magic",
                          "Expected 'IRTB'");
    }
    p += 4;

    // Check version
    if (*p != kVersion) {
        return make_error(ErrorCode::IoFailed, "Unsupported template version",
                          "Expected version " + std::to_string(kVersion) +
                              ", got " + std::to_string(*p));
    }
    p += 1;

    // Skip flags + reserved
    p += 3;

    PackedIrisCode code;

    // Read code_bits
    for (size_t i = 0; i < PackedIrisCode::kNumWords; ++i) {
        uint64_t w = 0;
        for (int b = 0; b < 8; ++b) {
            w |= static_cast<uint64_t>(*p++) << (b * 8);
        }
        code.code_bits[i] = w;
    }

    // Read mask_bits
    for (size_t i = 0; i < PackedIrisCode::kNumWords; ++i) {
        uint64_t w = 0;
        for (int b = 0; b < 8; ++b) {
            w |= static_cast<uint64_t>(*p++) << (b * 8);
        }
        code.mask_bits[i] = w;
    }

    return code;
}

// --- Python-compatible format ---

namespace {

/// Extract a single bit from PackedIrisCode at position (row, col, channel).
bool get_bit(const PackedIrisCode& code, size_t row, size_t col, size_t ch) {
    const size_t linear = row * PackedIrisCode::kBitsPerRow + col * 2 + ch;
    const size_t word = linear / 64;
    const size_t bit = linear % 64;
    return (code.code_bits[word] >> bit) & 1;
}

/// Extract a single bit from the mask_bits of a PackedIrisCode.
bool get_mask_bit(const PackedIrisCode& code, size_t row, size_t col, size_t ch) {
    const size_t linear = row * PackedIrisCode::kBitsPerRow + col * 2 + ch;
    const size_t word = linear / 64;
    const size_t bit = linear % 64;
    return (code.mask_bits[word] >> bit) & 1;
}

/// Set a single bit in PackedIrisCode.code_bits at position (row, col, channel).
void set_bit(PackedIrisCode& code, size_t row, size_t col, size_t ch, bool val) {
    const size_t linear = row * PackedIrisCode::kBitsPerRow + col * 2 + ch;
    const size_t word = linear / 64;
    const size_t bit = linear % 64;
    if (val) {
        code.code_bits[word] |= (uint64_t{1} << bit);
    } else {
        code.code_bits[word] &= ~(uint64_t{1} << bit);
    }
}

/// Set a single bit in PackedIrisCode.mask_bits.
void set_mask_bit(PackedIrisCode& code, size_t row, size_t col, size_t ch, bool val) {
    const size_t linear = row * PackedIrisCode::kBitsPerRow + col * 2 + ch;
    const size_t word = linear / 64;
    const size_t bit = linear % 64;
    if (val) {
        code.mask_bits[word] |= (uint64_t{1} << bit);
    } else {
        code.mask_bits[word] &= ~(uint64_t{1} << bit);
    }
}

/// Pack bits (MSB first, matching numpy's np.packbits) into bytes.
/// `bits` is a flat array of 0/1 values.
std::vector<uint8_t> packbits(const std::vector<uint8_t>& bits) {
    const size_t n_bytes = (bits.size() + 7) / 8;
    std::vector<uint8_t> packed(n_bytes, 0);
    for (size_t i = 0; i < bits.size(); ++i) {
        if (bits[i]) {
            packed[i / 8] |= static_cast<uint8_t>(0x80u >> (i % 8));
        }
    }
    return packed;
}

/// Unpack bytes to bits (MSB first, matching numpy's np.unpackbits).
std::vector<uint8_t> unpackbits(const std::vector<uint8_t>& packed, size_t total_bits) {
    std::vector<uint8_t> bits(total_bits, 0);
    for (size_t i = 0; i < total_bits && i / 8 < packed.size(); ++i) {
        bits[i] = (packed[i / 8] >> (7 - (i % 8))) & 1;
    }
    return bits;
}

}  // namespace

std::string TemplateSerializer::to_python_format(const IrisTemplate& tmpl) {
    const size_t n_wavelets = tmpl.iris_codes.size();
    const size_t rows = PackedIrisCode::kRows;
    const size_t cols = PackedIrisCode::kCols;
    const size_t channels = PackedIrisCode::kChannels;

    // Total bits in the old format: rows * cols * n_wavelets * channels
    const size_t total_bits = rows * cols * n_wavelets * channels;

    // Build flat bit arrays in Python's C-order: (rows, cols, n_wavelets, channels)
    std::vector<uint8_t> iris_bits(total_bits, 0);
    std::vector<uint8_t> mask_bits_flat(total_bits, 0);

    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            for (size_t w = 0; w < n_wavelets; ++w) {
                for (size_t ch = 0; ch < channels; ++ch) {
                    const size_t flat_idx =
                        r * (cols * n_wavelets * channels) +
                        c * (n_wavelets * channels) +
                        w * channels + ch;
                    iris_bits[flat_idx] =
                        get_bit(tmpl.iris_codes[w], r, c, ch) ? 1 : 0;
                    mask_bits_flat[flat_idx] =
                        get_mask_bit(tmpl.iris_codes[w], r, c, ch) ? 1 : 0;
                }
            }
        }
    }

    // Pack bits MSB-first (matching np.packbits)
    auto iris_packed = packbits(iris_bits);
    auto mask_packed = packbits(mask_bits_flat);

    // Base64 encode
    auto iris_b64 = base64::encode(iris_packed);
    auto mask_b64 = base64::encode(mask_packed);

    // Build JSON
    nlohmann::json j;
    j["iris_codes"] = iris_b64;
    j["mask_codes"] = mask_b64;
    j["iris_code_version"] = tmpl.iris_code_version;

    return j.dump();
}

Result<IrisTemplate> TemplateSerializer::from_python_format(std::string_view json_str) {
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(json_str);
    } catch (const nlohmann::json::exception& e) {
        return make_error(ErrorCode::IoFailed, "Invalid JSON", e.what());
    }

    if (!j.contains("iris_codes") || !j.contains("mask_codes")) {
        return make_error(ErrorCode::IoFailed, "Missing iris_codes or mask_codes in JSON");
    }

    auto iris_b64 = j["iris_codes"].get<std::string>();
    auto mask_b64 = j["mask_codes"].get<std::string>();

    // Decode base64
    auto iris_packed = base64::decode(iris_b64);
    if (!iris_packed.has_value()) {
        return make_error(ErrorCode::IoFailed, "Failed to decode iris_codes base64",
                          iris_packed.error().message);
    }

    auto mask_packed = base64::decode(mask_b64);
    if (!mask_packed.has_value()) {
        return make_error(ErrorCode::IoFailed, "Failed to decode mask_codes base64",
                          mask_packed.error().message);
    }

    // Determine number of wavelets from the packed data size.
    // Total bits = rows * cols * n_wavelets * channels
    // Packed bytes = ceil(total_bits / 8)
    // With rows=16, cols=256, channels=2: bits_per_wavelet = 16*256*2 = 8192
    // packed_bytes_per_wavelet = 1024
    constexpr size_t kBitsPerWavelet = PackedIrisCode::kTotalBits;  // 8192
    constexpr size_t kBytesPerWavelet = kBitsPerWavelet / 8;        // 1024

    if (iris_packed->size() % kBytesPerWavelet != 0) {
        return make_error(ErrorCode::IoFailed,
                          "Iris codes size not a multiple of wavelet size",
                          std::to_string(iris_packed->size()) + " bytes");
    }

    const size_t n_wavelets = iris_packed->size() / kBytesPerWavelet;
    const size_t rows = PackedIrisCode::kRows;
    const size_t cols = PackedIrisCode::kCols;
    const size_t channels = PackedIrisCode::kChannels;
    const size_t total_bits = rows * cols * n_wavelets * channels;

    // Unpack bits MSB-first
    auto iris_bits = unpackbits(*iris_packed, total_bits);
    auto mask_bits_flat = unpackbits(*mask_packed, total_bits);

    // Rebuild PackedIrisCodes from Python's C-order: (rows, cols, n_wavelets, channels)
    IrisTemplate tmpl;
    tmpl.iris_codes.resize(n_wavelets);
    tmpl.mask_codes.resize(n_wavelets);

    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            for (size_t w = 0; w < n_wavelets; ++w) {
                for (size_t ch = 0; ch < channels; ++ch) {
                    const size_t flat_idx =
                        r * (cols * n_wavelets * channels) +
                        c * (n_wavelets * channels) +
                        w * channels + ch;
                    set_bit(tmpl.iris_codes[w], r, c, ch,
                            iris_bits[flat_idx] != 0);
                    set_mask_bit(tmpl.iris_codes[w], r, c, ch,
                                 mask_bits_flat[flat_idx] != 0);
                }
            }
        }
    }

    // Also populate mask_codes for consistency (mask in both code_bits and mask_bits)
    for (size_t w = 0; w < n_wavelets; ++w) {
        tmpl.mask_codes[w].code_bits = tmpl.iris_codes[w].mask_bits;
        tmpl.mask_codes[w].mask_bits = tmpl.iris_codes[w].mask_bits;
    }

    if (j.contains("iris_code_version")) {
        tmpl.iris_code_version = j["iris_code_version"].get<std::string>();
    }

    return tmpl;
}

}  // namespace iris
