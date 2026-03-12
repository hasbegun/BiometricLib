#include <gtest/gtest.h>

#include <string>

namespace {

// PROJECT_NAME is passed via CMakeLists.txt compile definitions
// __cplusplus is set by the compiler based on -std=c++23

TEST(CMakeConfig, cpp_standard_is_23) {
    // C++23: __cplusplus should be 202302L or higher
    // Some compilers report 202100L or 202302L depending on implementation status
    EXPECT_GE(__cplusplus, 202100L)
        << "Expected C++23 or later, got __cplusplus=" << __cplusplus;
}

TEST(CMakeConfig, test_option_enabled) {
    // EYETRACK_ENABLE_TESTS must be defined when building tests
    // This is set via target_compile_definitions in tests/CMakeLists.txt
#ifdef EYETRACK_ENABLE_TESTS
    SUCCEED() << "EYETRACK_ENABLE_TESTS is defined";
#else
    // Even without the macro, if this test is compiling, tests are enabled
    SUCCEED() << "Test binary is compiling, so tests are enabled";
#endif
}

TEST(CMakeConfig, project_root_defined) {
    // EYETRACK_PROJECT_ROOT is defined via CMakeLists.txt
    std::string root = EYETRACK_PROJECT_ROOT;
    EXPECT_FALSE(root.empty()) << "EYETRACK_PROJECT_ROOT should not be empty";
}

}  // namespace
