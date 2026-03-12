#include <iris/utils/base64.hpp>

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace iris::test {

// --- RFC 4648 test vectors ---

TEST(Base64, EmptyRoundTrip) {
    std::vector<uint8_t> empty;
    auto encoded = base64::encode(empty);
    EXPECT_EQ(encoded, "");

    auto decoded = base64::decode(encoded);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_TRUE(decoded->empty());
}

TEST(Base64, Rfc4648TestVectors) {
    // RFC 4648 §10 test vectors
    struct TestCase {
        std::string input;
        std::string expected;
    };
    const TestCase cases[] = {
        {"", ""},
        {"f", "Zg=="},
        {"fo", "Zm8="},
        {"foo", "Zm9v"},
        {"foob", "Zm9vYg=="},
        {"fooba", "Zm9vYmE="},
        {"foobar", "Zm9vYmFy"},
    };

    for (const auto& tc : cases) {
        std::vector<uint8_t> data(tc.input.begin(), tc.input.end());
        auto encoded = base64::encode(data);
        EXPECT_EQ(encoded, tc.expected) << "encode(\"" << tc.input << "\")";

        auto decoded = base64::decode(encoded);
        ASSERT_TRUE(decoded.has_value()) << "decode(\"" << encoded << "\")";
        std::string recovered(decoded->begin(), decoded->end());
        EXPECT_EQ(recovered, tc.input) << "decode round-trip for \"" << tc.input << "\"";
    }
}

TEST(Base64, BinaryDataRoundTrip) {
    // All 256 byte values
    std::vector<uint8_t> data(256);
    std::iota(data.begin(), data.end(), uint8_t{0});

    auto encoded = base64::encode(data);
    EXPECT_FALSE(encoded.empty());

    auto decoded = base64::decode(encoded);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(*decoded, data);
}

TEST(Base64, InvalidInput) {
    // Invalid character
    auto result = base64::decode("abc!");
    EXPECT_FALSE(result.has_value());

    // Wrong length (not multiple of 4)
    result = base64::decode("abc");
    EXPECT_FALSE(result.has_value());
}

TEST(Base64, PaddingVariants) {
    // 0 pad chars: input length divisible by 3
    {
        std::vector<uint8_t> data = {0x61, 0x62, 0x63};  // "abc"
        auto encoded = base64::encode(data);
        EXPECT_EQ(encoded, "YWJj");  // no padding
        auto decoded = base64::decode(encoded);
        ASSERT_TRUE(decoded.has_value());
        EXPECT_EQ(*decoded, data);
    }
    // 1 pad char: input length % 3 == 2
    {
        std::vector<uint8_t> data = {0x61, 0x62};  // "ab"
        auto encoded = base64::encode(data);
        EXPECT_EQ(encoded, "YWI=");
        auto decoded = base64::decode(encoded);
        ASSERT_TRUE(decoded.has_value());
        EXPECT_EQ(*decoded, data);
    }
    // 2 pad chars: input length % 3 == 1
    {
        std::vector<uint8_t> data = {0x61};  // "a"
        auto encoded = base64::encode(data);
        EXPECT_EQ(encoded, "YQ==");
        auto decoded = base64::decode(encoded);
        ASSERT_TRUE(decoded.has_value());
        EXPECT_EQ(*decoded, data);
    }
}

TEST(Base64, LargeDataRoundTrip) {
    // 1 KB of pseudo-random data
    std::vector<uint8_t> data(1024);
    uint8_t val = 0;
    for (auto& b : data) {
        b = val;
        val = static_cast<uint8_t>(val * 7u + 13u);
    }

    auto encoded = base64::encode(data);
    auto decoded = base64::decode(encoded);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(*decoded, data);
}

TEST(Base64, PackedIrisCodeRoundTrip) {
    // Simulate encoding 1024 bytes (size of code_bits: 128 * 8 = 1024)
    std::vector<uint8_t> data(1024);
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<uint8_t>(i & 0xFF);
    }

    auto encoded = base64::encode(data);
    auto decoded = base64::decode(encoded);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->size(), 1024u);
    EXPECT_EQ(*decoded, data);
}

}  // namespace iris::test
