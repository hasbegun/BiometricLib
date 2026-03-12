#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>

// Platform-specific intrinsics
#if defined(__AVX512VPOPCNTDQ__) && defined(__AVX512F__)
#include <immintrin.h>
#elif defined(__AVX2__)
#include <immintrin.h>
#elif defined(__ARM_NEON)
#include <arm_neon.h>
#endif

namespace iris::simd {

/// Compute popcount(a[i] XOR b[i]) summed over n_words uint64_t elements.
/// This is the core inner loop for Hamming distance computation.
///
/// Compile-time dispatched to the best available SIMD instruction set.
/// All variants produce identical results for the same input.

#if defined(__AVX512VPOPCNTDQ__) && defined(__AVX512F__)

// AVX-512 VPOPCNT: process 512 bits (8 uint64_t) per iteration
inline size_t popcount_xor(const uint64_t* a, const uint64_t* b, size_t n_words) {
    __m512i total = _mm512_setzero_si512();
    size_t i = 0;

    for (; i + 8 <= n_words; i += 8) {
        __m512i va = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(a + i));
        __m512i vb = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(b + i));
        __m512i vxor = _mm512_xor_si512(va, vb);
        total = _mm512_add_epi64(total, _mm512_popcnt_epi64(vxor));
    }

    // Horizontal sum of 8 x uint64_t lanes
    size_t result = _mm512_reduce_add_epi64(total);

    // Scalar tail
    for (; i < n_words; ++i) {
        result += static_cast<size_t>(std::popcount(a[i] ^ b[i]));
    }
    return result;
}

#elif defined(__AVX2__)

// AVX2 Harley-Seal popcount using lookup table approach.
// Processes 256 bits (4 uint64_t) per iteration.
namespace detail {

inline __m256i popcount_avx2(__m256i v) {
    const __m256i lookup = _mm256_setr_epi8(
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4);
    const __m256i low_mask = _mm256_set1_epi8(0x0F);
    __m256i lo = _mm256_and_si256(v, low_mask);
    __m256i hi = _mm256_and_si256(_mm256_srli_epi16(v, 4), low_mask);
    __m256i popcnt_lo = _mm256_shuffle_epi8(lookup, lo);
    __m256i popcnt_hi = _mm256_shuffle_epi8(lookup, hi);
    return _mm256_add_epi8(popcnt_lo, popcnt_hi);
}

}  // namespace detail

inline size_t popcount_xor(const uint64_t* a, const uint64_t* b, size_t n_words) {
    __m256i total = _mm256_setzero_si256();
    size_t i = 0;

    for (; i + 4 <= n_words; i += 4) {
        __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(a + i));
        __m256i vb = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(b + i));
        __m256i vxor = _mm256_xor_si256(va, vb);
        total = _mm256_add_epi8(total, detail::popcount_avx2(vxor));
    }

    // Horizontal sum: sum bytes to 64-bit lanes, then extract
    __m256i sum64 = _mm256_sad_epu8(total, _mm256_setzero_si256());
    size_t result = 0;
    // Extract 4 x 64-bit values
    result += static_cast<size_t>(_mm256_extract_epi64(sum64, 0));
    result += static_cast<size_t>(_mm256_extract_epi64(sum64, 1));
    result += static_cast<size_t>(_mm256_extract_epi64(sum64, 2));
    result += static_cast<size_t>(_mm256_extract_epi64(sum64, 3));

    // Scalar tail
    for (; i < n_words; ++i) {
        result += static_cast<size_t>(std::popcount(a[i] ^ b[i]));
    }
    return result;
}

#elif defined(__ARM_NEON)

// ARM NEON with cnt instruction: process 128 bits (2 uint64_t) per iteration.
inline size_t popcount_xor(const uint64_t* a, const uint64_t* b, size_t n_words) {
    uint64x2_t total = vdupq_n_u64(0);
    size_t i = 0;

    for (; i + 2 <= n_words; i += 2) {
        uint64x2_t va = vld1q_u64(a + i);
        uint64x2_t vb = vld1q_u64(b + i);
        uint64x2_t vxor = veorq_u64(va, vb);
        // cnt counts bits per byte, then accumulate
        uint8x16_t cnt = vcntq_u8(vreinterpretq_u8_u64(vxor));
        // Sum bytes pairwise into 64-bit lanes
        total = vaddq_u64(total, vpaddlq_u32(vpaddlq_u16(vpaddlq_u8(cnt))));
    }

    size_t result = vgetq_lane_u64(total, 0) + vgetq_lane_u64(total, 1);

    // Scalar tail
    for (; i < n_words; ++i) {
        result += static_cast<size_t>(std::popcount(a[i] ^ b[i]));
    }
    return result;
}

#else

// Scalar fallback using C++20 std::popcount.
inline size_t popcount_xor(const uint64_t* a, const uint64_t* b, size_t n_words) {
    size_t result = 0;
    for (size_t i = 0; i < n_words; ++i) {
        result += static_cast<size_t>(std::popcount(a[i] ^ b[i]));
    }
    return result;
}

#endif

/// Compute popcount(a[i] AND b[i]) summed over n_words.
/// Used for counting valid mask bits.
inline size_t popcount_and(const uint64_t* a, const uint64_t* b, size_t n_words) {
    size_t result = 0;
    for (size_t i = 0; i < n_words; ++i) {
        result += static_cast<size_t>(std::popcount(a[i] & b[i]));
    }
    return result;
}

/// Compute popcount((a[i] XOR b[i]) AND c[i]) summed over n_words.
/// Used for masked Hamming distance: XOR the codes, then AND with combined mask.
inline size_t popcount_xor_and(
    const uint64_t* a,
    const uint64_t* b,
    const uint64_t* mask,
    size_t n_words) {
    size_t result = 0;
    for (size_t i = 0; i < n_words; ++i) {
        result += static_cast<size_t>(std::popcount((a[i] ^ b[i]) & mask[i]));
    }
    return result;
}

}  // namespace iris::simd
