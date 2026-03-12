#pragma once

#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <utility>

#include <opencv2/core.hpp>

namespace iris {

/// Bit-packed iris code for SIMD-friendly Hamming distance computation.
///
/// Each iris code has 16 rows x 256 cols x 2 channels = 8192 bits.
/// Packed into 128 x uint64_t words, 64-byte aligned for AVX-512.
///
/// Bit indexing: bit at (row, col, channel) maps to:
///   linear_index = row * 512 + col * 2 + channel
///   word = linear_index / 64
///   bit  = linear_index % 64
struct PackedIrisCode {
    static constexpr size_t kTotalBits = 8192;
    static constexpr size_t kNumWords = 128;
    static constexpr size_t kRows = 16;
    static constexpr size_t kCols = 256;
    static constexpr size_t kChannels = 2;
    static constexpr size_t kBitsPerRow = kCols * kChannels;  // 512
    static constexpr size_t kWordsPerRow = kBitsPerRow / 64;  // 8

    alignas(64) std::array<uint64_t, kNumWords> code_bits{};
    alignas(64) std::array<uint64_t, kNumWords> mask_bits{};

    /// Pack from cv::Mat (CV_8UC1, logically 16x512 with one byte per bit).
    /// `code` and `mask` must each have total elements == kTotalBits.
    static PackedIrisCode from_mat(const cv::Mat& code, const cv::Mat& mask);

    /// Unpack to cv::Mat pair (CV_8UC1, 16x512) for comparison/serialization.
    [[nodiscard]] std::pair<cv::Mat, cv::Mat> to_mat() const;

    /// Circular column shift. Each column is 2 bits wide, so a shift of `s`
    /// columns = 2*s bit positions. Applied per-row (512-bit circular shift).
    [[nodiscard]] PackedIrisCode rotate(int shift) const;
};

}  // namespace iris
