#include <iris/core/iris_code_packed.hpp>

#include <cassert>
#include <cstring>

namespace iris {

PackedIrisCode PackedIrisCode::from_mat(const cv::Mat& code, const cv::Mat& mask) {
    assert(code.total() == kTotalBits);
    assert(mask.total() == kTotalBits);
    assert(code.type() == CV_8UC1);
    assert(mask.type() == CV_8UC1);

    PackedIrisCode packed{};

    const auto* code_ptr = code.ptr<uint8_t>();
    const auto* mask_ptr = mask.ptr<uint8_t>();

    for (size_t i = 0; i < kTotalBits; ++i) {
        const size_t word = i / 64;
        const size_t bit = i % 64;
        if (code_ptr[i] != 0) {
            packed.code_bits[word] |= (uint64_t{1} << bit);
        }
        if (mask_ptr[i] != 0) {
            packed.mask_bits[word] |= (uint64_t{1} << bit);
        }
    }

    return packed;
}

std::pair<cv::Mat, cv::Mat> PackedIrisCode::to_mat() const {
    cv::Mat code(static_cast<int>(kRows), static_cast<int>(kBitsPerRow), CV_8UC1, cv::Scalar(0));
    cv::Mat mask(static_cast<int>(kRows), static_cast<int>(kBitsPerRow), CV_8UC1, cv::Scalar(0));

    auto* code_ptr = code.ptr<uint8_t>();
    auto* mask_ptr = mask.ptr<uint8_t>();

    for (size_t i = 0; i < kTotalBits; ++i) {
        const size_t word = i / 64;
        const size_t bit = i % 64;
        code_ptr[i] = static_cast<uint8_t>((code_bits[word] >> bit) & 1u);
        mask_ptr[i] = static_cast<uint8_t>((mask_bits[word] >> bit) & 1u);
    }

    return {code, mask};
}

namespace {

/// Circular left-shift of a 512-bit value stored in 8 x uint64_t words.
/// `shift` is in bits, can be negative (right shift).
void circular_shift_512(const uint64_t* src, uint64_t* dst, int shift) {
    constexpr int kRowBits = 512;
    constexpr int kWords = 8;

    // Normalize to positive shift in [0, 512)
    shift = ((shift % kRowBits) + kRowBits) % kRowBits;
    if (shift == 0) {
        std::memcpy(dst, src, kWords * sizeof(uint64_t));
        return;
    }

    const int word_shift = shift / 64;
    const int bit_shift = shift % 64;

    for (int i = 0; i < kWords; ++i) {
        const int src_lo = (i - word_shift + kWords) % kWords;
        const int src_hi = (src_lo - 1 + kWords) % kWords;

        if (bit_shift == 0) {
            dst[i] = src[src_lo];
        } else {
            dst[i] = (src[src_lo] << bit_shift) | (src[src_hi] >> (64 - bit_shift));
        }
    }
}

}  // namespace

PackedIrisCode PackedIrisCode::rotate(int shift) const {
    if (shift == 0) {
        return *this;
    }

    PackedIrisCode result{};
    const int bit_shift = shift * static_cast<int>(kChannels);  // 2 bits per column

    for (size_t row = 0; row < kRows; ++row) {
        const size_t base = row * kWordsPerRow;
        circular_shift_512(&code_bits[base], &result.code_bits[base], bit_shift);
        circular_shift_512(&mask_bits[base], &result.mask_bits[base], bit_shift);
    }

    return result;
}

}  // namespace iris
