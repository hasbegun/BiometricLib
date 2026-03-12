#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace {

// Project root is passed via CMake define or defaults to relative path
#ifndef EYETRACK_PROJECT_ROOT
#define EYETRACK_PROJECT_ROOT "."
#endif

const fs::path kProjectRoot{EYETRACK_PROJECT_ROOT};

TEST(Scaffolding, directory_structure_exists) {
    const std::vector<std::string> expected_dirs = {
        "include/eyetrack/core",
        "include/eyetrack/nodes",
        "include/eyetrack/pipeline",
        "include/eyetrack/comm",
        "include/eyetrack/utils",
        "src/core",
        "src/nodes",
        "src/pipeline",
        "src/comm",
        "src/utils",
        "tests/unit",
        "tests/integration",
        "tests/e2e",
        "tests/bench",
        "tests/fixtures",
        "proto",
        "cmake",
        "config",
        "models",
        "tools",
    };

    for (const auto& dir : expected_dirs) {
        auto full_path = kProjectRoot / dir;
        EXPECT_TRUE(fs::is_directory(full_path))
            << "Missing directory: " << full_path.string();
    }
}

TEST(Scaffolding, clang_format_present) {
    auto path = kProjectRoot / ".clang-format";
    ASSERT_TRUE(fs::exists(path)) << ".clang-format not found at " << path.string();

    std::ifstream file(path);
    ASSERT_TRUE(file.is_open());

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    EXPECT_NE(content.find("eyetrack"), std::string::npos)
        << ".clang-format should reference eyetrack namespace in IncludeCategories";

    EXPECT_NE(content.find("c++23"), std::string::npos)
        << ".clang-format should specify C++23 standard";
}

TEST(Scaffolding, clang_tidy_present) {
    auto path = kProjectRoot / ".clang-tidy";
    ASSERT_TRUE(fs::exists(path)) << ".clang-tidy not found at " << path.string();

    std::ifstream file(path);
    ASSERT_TRUE(file.is_open());

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    EXPECT_NE(content.find("eyetrack"), std::string::npos)
        << ".clang-tidy HeaderFilterRegex should reference eyetrack";
}

TEST(Scaffolding, gitignore_excludes_build) {
    auto path = kProjectRoot / ".gitignore";
    ASSERT_TRUE(fs::exists(path)) << ".gitignore not found at " << path.string();

    std::ifstream file(path);
    ASSERT_TRUE(file.is_open());

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    EXPECT_NE(content.find("build/"), std::string::npos)
        << ".gitignore should exclude build/";

    EXPECT_NE(content.find("*.onnx"), std::string::npos)
        << ".gitignore should exclude ONNX model files";

    EXPECT_NE(content.find("CMakeFiles/"), std::string::npos)
        << ".gitignore should exclude CMakeFiles/";
}

}  // namespace
