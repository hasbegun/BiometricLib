#include <iris/utils/base64.hpp>

#include <array>

namespace iris::base64 {

namespace {

// RFC 4648 standard alphabet
constexpr std::array<char, 64> kEncodeTable = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

// Decode table: maps ASCII byte to 6-bit value, 0xFF = invalid, 0xFE = padding
constexpr auto make_decode_table() {
    std::array<uint8_t, 256> table{};
    for (auto& v : table) v = 0xFF;
    for (uint8_t i = 0; i < 64; ++i) {
        table[static_cast<uint8_t>(kEncodeTable[i])] = i;
    }
    table[static_cast<uint8_t>('=')] = 0xFE;
    return table;
}

constexpr auto kDecodeTable = make_decode_table();

}  // namespace

std::string encode(std::span<const uint8_t> data) {
    if (data.empty()) return {};

    const size_t full_triples = data.size() / 3;
    const size_t remainder = data.size() % 3;
    const size_t output_size = 4 * ((data.size() + 2) / 3);

    std::string result;
    result.reserve(output_size);

    size_t i = 0;
    for (size_t t = 0; t < full_triples; ++t, i += 3) {
        const uint32_t triple =
            (static_cast<uint32_t>(data[i]) << 16) |
            (static_cast<uint32_t>(data[i + 1]) << 8) |
            static_cast<uint32_t>(data[i + 2]);
        result += kEncodeTable[(triple >> 18) & 0x3F];
        result += kEncodeTable[(triple >> 12) & 0x3F];
        result += kEncodeTable[(triple >> 6) & 0x3F];
        result += kEncodeTable[triple & 0x3F];
    }

    if (remainder == 1) {
        const uint32_t val = static_cast<uint32_t>(data[i]) << 16;
        result += kEncodeTable[(val >> 18) & 0x3F];
        result += kEncodeTable[(val >> 12) & 0x3F];
        result += '=';
        result += '=';
    } else if (remainder == 2) {
        const uint32_t val =
            (static_cast<uint32_t>(data[i]) << 16) |
            (static_cast<uint32_t>(data[i + 1]) << 8);
        result += kEncodeTable[(val >> 18) & 0x3F];
        result += kEncodeTable[(val >> 12) & 0x3F];
        result += kEncodeTable[(val >> 6) & 0x3F];
        result += '=';
    }

    return result;
}

Result<std::vector<uint8_t>> decode(std::string_view encoded) {
    if (encoded.empty()) return std::vector<uint8_t>{};

    // Strip trailing whitespace/newlines
    while (!encoded.empty() &&
           (encoded.back() == '\n' || encoded.back() == '\r' ||
            encoded.back() == ' ')) {
        encoded.remove_suffix(1);
    }

    if (encoded.size() % 4 != 0) {
        return make_error(ErrorCode::IoFailed, "Invalid base64 length",
                          "Input length must be a multiple of 4");
    }

    // Count padding
    size_t padding = 0;
    if (encoded.size() >= 2 && encoded[encoded.size() - 1] == '=') {
        ++padding;
        if (encoded[encoded.size() - 2] == '=') ++padding;
    }

    const size_t output_size = (encoded.size() / 4) * 3 - padding;
    std::vector<uint8_t> result;
    result.reserve(output_size);

    for (size_t i = 0; i < encoded.size(); i += 4) {
        uint32_t sextet[4];
        for (int j = 0; j < 4; ++j) {
            const uint8_t ch = static_cast<uint8_t>(encoded[i + static_cast<size_t>(j)]);
            const uint8_t val = kDecodeTable[ch];
            if (val == 0xFF) {
                return make_error(
                    ErrorCode::IoFailed, "Invalid base64 character",
                    std::string("Invalid character at position ") +
                        std::to_string(i + static_cast<size_t>(j)));
            }
            sextet[j] = (val == 0xFE) ? 0u : static_cast<uint32_t>(val);
        }

        const uint32_t triple =
            (sextet[0] << 18) | (sextet[1] << 12) | (sextet[2] << 6) | sextet[3];

        result.push_back(static_cast<uint8_t>((triple >> 16) & 0xFF));

        // Check if we're in the last quad with padding
        const bool is_last = (i + 4 == encoded.size());
        if (!is_last || padding < 2) {
            result.push_back(static_cast<uint8_t>((triple >> 8) & 0xFF));
        }
        if (!is_last || padding == 0) {
            result.push_back(static_cast<uint8_t>(triple & 0xFF));
        }
    }

    return result;
}

}  // namespace iris::base64
