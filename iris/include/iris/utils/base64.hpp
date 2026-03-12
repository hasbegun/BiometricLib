#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <iris/core/error.hpp>

namespace iris::base64 {

/// Encode binary data to base64 (RFC 4648).
std::string encode(std::span<const uint8_t> data);

/// Decode base64 string to binary data. Returns error on invalid input.
Result<std::vector<uint8_t>> decode(std::string_view encoded);

}  // namespace iris::base64
