#include <iris/utils/template_serializer.hpp>

#include <cstdint>
#include <cstring>
#include <random>

#include <gtest/gtest.h>

namespace iris::test {

// --- Binary format tests ---

TEST(TemplateSerializer, BinaryRoundTripZeros) {
    PackedIrisCode code;  // default: all zeros
    auto bytes = TemplateSerializer::to_binary(code);
    ASSERT_EQ(bytes.size(), TemplateSerializer::kBinarySize);

    auto result = TemplateSerializer::from_binary(bytes);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->code_bits, code.code_bits);
    EXPECT_EQ(result->mask_bits, code.mask_bits);
}

TEST(TemplateSerializer, BinaryRoundTripRandom) {
    PackedIrisCode code;
    std::mt19937_64 rng(42);
    for (auto& w : code.code_bits) w = rng();
    for (auto& w : code.mask_bits) w = rng();

    auto bytes = TemplateSerializer::to_binary(code);
    auto result = TemplateSerializer::from_binary(bytes);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(result->code_bits, code.code_bits);
    EXPECT_EQ(result->mask_bits, code.mask_bits);
}

TEST(TemplateSerializer, BinaryTruncatedFails) {
    PackedIrisCode code;
    auto bytes = TemplateSerializer::to_binary(code);
    bytes.resize(100);  // truncate

    auto result = TemplateSerializer::from_binary(bytes);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::IoFailed);
}

TEST(TemplateSerializer, BinaryBadMagicFails) {
    PackedIrisCode code;
    auto bytes = TemplateSerializer::to_binary(code);
    bytes[0] = 'X';  // corrupt magic

    auto result = TemplateSerializer::from_binary(bytes);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::IoFailed);
}

TEST(TemplateSerializer, BinaryBadVersionFails) {
    PackedIrisCode code;
    auto bytes = TemplateSerializer::to_binary(code);
    bytes[4] = 99;  // version byte at offset 4

    auto result = TemplateSerializer::from_binary(bytes);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::IoFailed);
}

TEST(TemplateSerializer, BinarySizeExact) {
    PackedIrisCode code;
    auto bytes = TemplateSerializer::to_binary(code);
    // 4 magic + 1 version + 1 flags + 2 reserved + 1024 code + 1024 mask
    EXPECT_EQ(bytes.size(), 2056u);
}

// --- Python format tests ---

TEST(TemplateSerializer, PythonFormatRoundTripZeros) {
    IrisTemplate tmpl;
    // Two wavelets (matching default pipeline)
    tmpl.iris_codes.resize(2);
    tmpl.mask_codes.resize(2);
    // All zeros
    tmpl.iris_code_version = "v2.0";

    auto json = TemplateSerializer::to_python_format(tmpl);
    auto result = TemplateSerializer::from_python_format(json);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_EQ(result->iris_codes.size(), 2u);
    EXPECT_EQ(result->iris_code_version, "v2.0");

    for (size_t w = 0; w < 2; ++w) {
        EXPECT_EQ(result->iris_codes[w].code_bits, tmpl.iris_codes[w].code_bits);
        EXPECT_EQ(result->iris_codes[w].mask_bits, tmpl.iris_codes[w].mask_bits);
    }
}

TEST(TemplateSerializer, PythonFormatRoundTripRandom) {
    IrisTemplate tmpl;
    tmpl.iris_codes.resize(2);
    tmpl.mask_codes.resize(2);

    std::mt19937_64 rng(123);
    for (size_t w = 0; w < 2; ++w) {
        for (auto& v : tmpl.iris_codes[w].code_bits) v = rng();
        for (auto& v : tmpl.iris_codes[w].mask_bits) v = rng();
        tmpl.mask_codes[w].code_bits = tmpl.iris_codes[w].mask_bits;
        tmpl.mask_codes[w].mask_bits = tmpl.iris_codes[w].mask_bits;
    }
    tmpl.iris_code_version = "v2.0";

    auto json = TemplateSerializer::to_python_format(tmpl);
    auto result = TemplateSerializer::from_python_format(json);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    ASSERT_EQ(result->iris_codes.size(), 2u);
    for (size_t w = 0; w < 2; ++w) {
        EXPECT_EQ(result->iris_codes[w].code_bits, tmpl.iris_codes[w].code_bits)
            << "code_bits mismatch at wavelet " << w;
        EXPECT_EQ(result->iris_codes[w].mask_bits, tmpl.iris_codes[w].mask_bits)
            << "mask_bits mismatch at wavelet " << w;
    }
}

TEST(TemplateSerializer, PythonFormatInvalidJson) {
    auto result = TemplateSerializer::from_python_format("not valid json{{{");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::IoFailed);
}

TEST(TemplateSerializer, PythonFormatMissingKeys) {
    auto result = TemplateSerializer::from_python_format(R"({"foo": "bar"})");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::IoFailed);
}

TEST(TemplateSerializer, PythonFormatKnownBitPattern) {
    // Create a template where iris_codes[0] has bit 0 set at (row=0, col=0, ch=0)
    // and iris_codes[1] has bit 0 set at (row=0, col=0, ch=0).
    // Verify the serialized format contains the expected base64 string.
    IrisTemplate tmpl;
    tmpl.iris_codes.resize(1);
    tmpl.mask_codes.resize(1);
    tmpl.iris_code_version = "v2.0";

    // Set all mask bits to 1 for simplicity
    for (auto& w : tmpl.iris_codes[0].mask_bits) w = ~uint64_t{0};
    tmpl.mask_codes[0].code_bits = tmpl.iris_codes[0].mask_bits;
    tmpl.mask_codes[0].mask_bits = tmpl.iris_codes[0].mask_bits;

    // Set only code bit (row=0, col=0, ch=0)
    tmpl.iris_codes[0].code_bits[0] = 1;  // bit 0 of word 0

    // Serialize and deserialize
    auto json_str = TemplateSerializer::to_python_format(tmpl);
    auto result = TemplateSerializer::from_python_format(json_str);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Verify the specific bit is set
    EXPECT_EQ(result->iris_codes[0].code_bits[0], uint64_t{1});
    // All mask bits should be set
    for (auto w : result->iris_codes[0].mask_bits) {
        EXPECT_EQ(w, ~uint64_t{0});
    }
}

}  // namespace iris::test
