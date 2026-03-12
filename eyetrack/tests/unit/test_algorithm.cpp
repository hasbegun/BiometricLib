#include <gtest/gtest.h>

#include <string_view>

#include <eyetrack/core/algorithm.hpp>
#include <eyetrack/core/error.hpp>

namespace {

using namespace eyetrack;

// Concrete CRTP algorithm for testing
class TestAlgorithm : public Algorithm<TestAlgorithm> {
public:
    static constexpr std::string_view kName = "TestAlgorithm";

    Result<int> run(int input) const { return input * 2; }
};

TEST(Algorithm, crtp_derived_calls_process) {
    TestAlgorithm algo;
    auto result = algo.run(21);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 42);
}

TEST(Algorithm, algorithm_name_returns_correct_string) {
    TestAlgorithm algo;
    EXPECT_EQ(algo.name(), "TestAlgorithm");
}

}  // namespace
