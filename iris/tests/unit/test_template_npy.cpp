#include <cstdint>
#include <filesystem>
#include <random>

#include <iris/core/iris_code_packed.hpp>

#include "../comparison/template_npy_utils.hpp"

#include <gtest/gtest.h>

namespace iris::test {

namespace {

struct TmpDir {
    std::filesystem::path path;
    explicit TmpDir(const std::string& name)
        : path(std::filesystem::temp_directory_path() / name) {
        std::filesystem::create_directories(path);
    }
    ~TmpDir() { std::filesystem::remove_all(path); }
};

}  // namespace

TEST(TemplateNpy, RoundTripZeros) {
    TmpDir dir("test_template_npy_zeros");
    auto code_path = dir.path / "code.npy";
    auto mask_path = dir.path / "mask.npy";

    PackedIrisCode original{};  // all zeros

    npy::write_packed_code(code_path, mask_path, original);
    auto result = npy::read_packed_code(code_path, mask_path);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_EQ(result->code_bits, original.code_bits);
    EXPECT_EQ(result->mask_bits, original.mask_bits);
}

TEST(TemplateNpy, RoundTripRandomBits) {
    TmpDir dir("test_template_npy_random");
    auto code_path = dir.path / "code.npy";
    auto mask_path = dir.path / "mask.npy";

    std::mt19937_64 rng(42);
    PackedIrisCode original;
    for (auto& w : original.code_bits) w = rng();
    for (auto& w : original.mask_bits) w = rng();

    npy::write_packed_code(code_path, mask_path, original);
    auto result = npy::read_packed_code(code_path, mask_path);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_EQ(result->code_bits, original.code_bits);
    EXPECT_EQ(result->mask_bits, original.mask_bits);
}

TEST(TemplateNpy, ReadNonexistentFileReturnsError) {
    auto result = npy::read_packed_code("/nonexistent/code.npy", "/nonexistent/mask.npy");
    EXPECT_FALSE(result.has_value());
}

}  // namespace iris::test
