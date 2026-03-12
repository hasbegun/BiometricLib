#include <iris/pipeline/pipeline_context.hpp>

#include <string>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

namespace iris::test {

// --- set/get round-trip ---

TEST(PipelineContext, SetGetRoundTrip) {
    PipelineContext ctx;
    ctx.set<int>("count", 42);

    auto result = ctx.get<int>("count");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(**result, 42);
}

TEST(PipelineContext, SetGetString) {
    PipelineContext ctx;
    ctx.set<std::string>("name", "test_node");

    auto result = ctx.get<std::string>("name");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(**result, "test_node");
}

// --- Missing key error ---

TEST(PipelineContext, GetMissingKeyReturnsError) {
    PipelineContext ctx;

    auto result = ctx.get<int>("nonexistent");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ConfigInvalid);
    EXPECT_NE(result.error().message.find("not found"), std::string::npos);
}

// --- Wrong type error ---

TEST(PipelineContext, GetWrongTypeReturnsError) {
    PipelineContext ctx;
    ctx.set<int>("count", 42);

    auto result = ctx.get<std::string>("count");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ConfigInvalid);
    EXPECT_NE(result.error().message.find("type mismatch"), std::string::npos);
}

// --- has() ---

TEST(PipelineContext, HasKey) {
    PipelineContext ctx;
    EXPECT_FALSE(ctx.has("foo"));

    ctx.set<int>("foo", 1);
    EXPECT_TRUE(ctx.has("foo"));
}

// --- get_indexed for pair ---

TEST(PipelineContext, GetIndexedPairFirst) {
    PipelineContext ctx;
    ctx.set("data", std::pair<int, double>{10, 3.14});

    auto result = ctx.get_indexed<0, std::pair<int, double>>("data");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(**result, 10);
}

TEST(PipelineContext, GetIndexedPairSecond) {
    PipelineContext ctx;
    ctx.set("data", std::pair<int, double>{10, 3.14});

    auto result = ctx.get_indexed<1, std::pair<int, double>>("data");
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(**result, 3.14);
}

TEST(PipelineContext, GetIndexedMissingKey) {
    PipelineContext ctx;

    auto result = ctx.get_indexed<0, std::pair<int, double>>("missing");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ConfigInvalid);
}

// --- keys() and size() ---

TEST(PipelineContext, KeysAndSize) {
    PipelineContext ctx;
    EXPECT_EQ(ctx.size(), 0u);
    EXPECT_TRUE(ctx.keys().empty());

    ctx.set<int>("a", 1);
    ctx.set<int>("b", 2);
    ctx.set<int>("c", 3);

    EXPECT_EQ(ctx.size(), 3u);
    auto k = ctx.keys();
    EXPECT_EQ(k.size(), 3u);
    EXPECT_TRUE(ctx.has("a"));
    EXPECT_TRUE(ctx.has("b"));
    EXPECT_TRUE(ctx.has("c"));
}

// --- Overwrite existing key ---

TEST(PipelineContext, OverwriteKey) {
    PipelineContext ctx;
    ctx.set<int>("x", 1);
    ctx.set<int>("x", 99);

    auto result = ctx.get<int>("x");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(**result, 99);
}

// --- clear() ---

TEST(PipelineContext, Clear) {
    PipelineContext ctx;
    ctx.set<int>("a", 1);
    ctx.set<int>("b", 2);
    EXPECT_EQ(ctx.size(), 2u);

    ctx.clear();
    EXPECT_EQ(ctx.size(), 0u);
    EXPECT_FALSE(ctx.has("a"));
}

// --- get_raw() ---

TEST(PipelineContext, GetRawReturnsPointer) {
    PipelineContext ctx;
    ctx.set<int>("val", 77);

    const std::any* raw = ctx.get_raw("val");
    ASSERT_NE(raw, nullptr);
    EXPECT_EQ(std::any_cast<int>(*raw), 77);
}

TEST(PipelineContext, GetRawMissingReturnsNull) {
    PipelineContext ctx;
    EXPECT_EQ(ctx.get_raw("nope"), nullptr);
}

// --- Move-only semantics ---

TEST(PipelineContext, MoveOnlySemantics) {
    static_assert(!std::is_copy_constructible_v<PipelineContext>,
                  "PipelineContext must not be copyable");
    static_assert(!std::is_copy_assignable_v<PipelineContext>,
                  "PipelineContext must not be copy-assignable");
    static_assert(std::is_move_constructible_v<PipelineContext>,
                  "PipelineContext must be movable");
    static_assert(std::is_move_assignable_v<PipelineContext>,
                  "PipelineContext must be move-assignable");

    PipelineContext ctx1;
    ctx1.set<int>("x", 42);

    PipelineContext ctx2 = std::move(ctx1);
    auto result = ctx2.get<int>("x");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(**result, 42);
}

}  // namespace iris::test
