#include <iris/nodes/matcher_simd.hpp>

#include <array>
#include <bit>
#include <cstdint>

#include <gtest/gtest.h>

using namespace iris::simd;

// Reference scalar implementation for validation
static size_t popcount_xor_scalar(const uint64_t* a, const uint64_t* b, size_t n) {
    size_t result = 0;
    for (size_t i = 0; i < n; ++i) {
        result += static_cast<size_t>(std::popcount(a[i] ^ b[i]));
    }
    return result;
}

TEST(SimdPopcount, AllZeros) {
    std::array<uint64_t, 128> a{};
    std::array<uint64_t, 128> b{};
    EXPECT_EQ(popcount_xor(a.data(), b.data(), 128), 0u);
}

TEST(SimdPopcount, AllOnes) {
    std::array<uint64_t, 128> a{};
    std::array<uint64_t, 128> b{};
    a.fill(0xFFFFFFFFFFFFFFFFULL);
    b.fill(0);
    EXPECT_EQ(popcount_xor(a.data(), b.data(), 128), 128u * 64u);
}

TEST(SimdPopcount, IdenticalArrays) {
    std::array<uint64_t, 128> a{};
    a.fill(0xDEADBEEFCAFEBABEULL);
    EXPECT_EQ(popcount_xor(a.data(), a.data(), 128), 0u);
}

TEST(SimdPopcount, KnownPattern) {
    std::array<uint64_t, 4> a = {0xFF00FF00FF00FF00ULL, 0, 0, 0};
    std::array<uint64_t, 4> b = {0x00FF00FF00FF00FFULL, 0, 0, 0};
    // XOR = 0xFFFFFFFFFFFFFFFF, popcount = 64
    EXPECT_EQ(popcount_xor(a.data(), b.data(), 4), 64u);
}

TEST(SimdPopcount, MatchesScalar) {
    // Test with pseudo-random data
    std::array<uint64_t, 128> a{};
    std::array<uint64_t, 128> b{};

    uint64_t seed = 0x12345678DEADBEEFULL;
    for (size_t i = 0; i < 128; ++i) {
        seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
        a[i] = seed;
        seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
        b[i] = seed;
    }

    size_t expected = popcount_xor_scalar(a.data(), b.data(), 128);
    size_t actual = popcount_xor(a.data(), b.data(), 128);
    EXPECT_EQ(actual, expected);
}

TEST(SimdPopcount, SmallArrays) {
    uint64_t a = 0xF0F0F0F0F0F0F0F0ULL;
    uint64_t b = 0x0F0F0F0F0F0F0F0FULL;
    EXPECT_EQ(popcount_xor(&a, &b, 1), 64u);
}

TEST(SimdPopcount, PopcountAnd) {
    std::array<uint64_t, 4> a = {0xFF00FF00FF00FF00ULL, 0xFFFFFFFFFFFFFFFFULL, 0, 0};
    std::array<uint64_t, 4> b = {0x00FF00FF00FF00FFULL, 0xFFFFFFFFFFFFFFFFULL, 0, 0};
    // AND of first word = 0, AND of second = all 1s
    EXPECT_EQ(popcount_and(a.data(), b.data(), 4), 64u);
}

TEST(SimdPopcount, PopcountXorAnd) {
    std::array<uint64_t, 4> a = {0xFF, 0, 0, 0};
    std::array<uint64_t, 4> b = {0x0F, 0, 0, 0};
    std::array<uint64_t, 4> mask = {0xFF, 0, 0, 0};
    // XOR = 0xF0, AND with mask = 0xF0, popcount = 4
    EXPECT_EQ(popcount_xor_and(a.data(), b.data(), mask.data(), 4), 4u);
}
