/// API hardening tests — verify no crashes on malformed input at public boundaries.
/// These complement existing unit tests by focusing on edge cases and garbage input.

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include <iris/core/config.hpp>
#include <iris/core/types.hpp>
#include <iris/utils/base64.hpp>
#include <iris/utils/template_serializer.hpp>

#include <gtest/gtest.h>

namespace iris::test {

// --- Base64 hardening ---

TEST(ApiHardening, Base64DecodeInvalidCharsNocrash) {
    auto result = base64::decode("!!!invalid!!!");
    EXPECT_FALSE(result.has_value());
}

TEST(ApiHardening, Base64DecodeNullBytesNocrash) {
    std::string with_nulls = "AAAA";
    with_nulls[2] = '\0';
    auto result = base64::decode(with_nulls);
    // Should either decode or error — no crash
    (void)result;
}

TEST(ApiHardening, Base64DecodeVeryLargeInput) {
    // 1 MB of valid-ish base64 — should not crash or OOM
    std::string large(1024 * 1024, 'A');
    auto result = base64::decode(large);
    EXPECT_TRUE(result.has_value());
}

// --- TemplateSerializer hardening ---

TEST(ApiHardening, BinaryDeserializeGarbageBytes) {
    std::mt19937 rng(42);
    std::vector<uint8_t> garbage(2056);
    for (auto& b : garbage) b = static_cast<uint8_t>(rng() & 0xFF);
    auto result = TemplateSerializer::from_binary(garbage);
    // Should error (bad magic) — no crash
    EXPECT_FALSE(result.has_value());
}

TEST(ApiHardening, BinaryDeserializeEmptyInput) {
    std::vector<uint8_t> empty;
    auto result = TemplateSerializer::from_binary(empty);
    EXPECT_FALSE(result.has_value());
}

TEST(ApiHardening, PythonFormatDeserializeEmptyString) {
    auto result = TemplateSerializer::from_python_format("");
    EXPECT_FALSE(result.has_value());
}

TEST(ApiHardening, PythonFormatDeserializeGarbageJson) {
    auto result = TemplateSerializer::from_python_format(
        R"({"iris_codes": [{"iris_code": "not-base64!!!"}]})");
    EXPECT_FALSE(result.has_value());
}

// --- Pipeline config hardening ---

TEST(ApiHardening, ParseEmptyYaml) {
    auto result = parse_pipeline_config("");
    // Should error cleanly — no crash
    EXPECT_FALSE(result.has_value());
}

TEST(ApiHardening, ParseMalformedYaml) {
    auto result = parse_pipeline_config("{{{{invalid yaml}}}}: ][");
    EXPECT_FALSE(result.has_value());
}

TEST(ApiHardening, ParseYamlWithNoNodes) {
    auto result = parse_pipeline_config(
        "metadata:\n  pipeline_name: test\n  iris_version: v1\npipeline: []\n");
    // May succeed with empty pipeline or fail — no crash
    (void)result;
}

}  // namespace iris::test
