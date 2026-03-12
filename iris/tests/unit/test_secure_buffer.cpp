#include <iris/crypto/secure_buffer.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include <gtest/gtest.h>

namespace iris::test {

TEST(SecureBuffer, ConstructWriteRead) {
    SecureBuffer buf(64);
    ASSERT_EQ(buf.size(), 64u);
    EXPECT_FALSE(buf.empty());

    auto data = buf.data();
    ASSERT_EQ(data.size(), 64u);

    // Initially zeroed
    for (auto b : data) {
        EXPECT_EQ(b, 0);
    }

    // Write data
    for (size_t i = 0; i < 64; ++i) {
        data[i] = static_cast<uint8_t>(i);
    }

    // Read back
    auto cdata = std::as_const(buf).data();
    for (size_t i = 0; i < 64; ++i) {
        EXPECT_EQ(cdata[i], static_cast<uint8_t>(i));
    }
}

TEST(SecureBuffer, MoveConstruct) {
    SecureBuffer a(128);
    auto ad = a.data();
    for (size_t i = 0; i < 128; ++i) {
        ad[i] = static_cast<uint8_t>(i & 0xFF);
    }

    SecureBuffer b(std::move(a));
    EXPECT_EQ(a.size(), 0u);  // NOLINT(bugprone-use-after-move)
    EXPECT_TRUE(a.empty());   // NOLINT(bugprone-use-after-move)
    EXPECT_EQ(b.size(), 128u);

    auto bd = b.data();
    for (size_t i = 0; i < 128; ++i) {
        EXPECT_EQ(bd[i], static_cast<uint8_t>(i & 0xFF));
    }
}

TEST(SecureBuffer, MoveAssign) {
    SecureBuffer a(64);
    SecureBuffer b(32);

    auto ad = a.data();
    for (auto& v : ad) v = 0xAA;

    b = std::move(a);
    EXPECT_EQ(a.size(), 0u);  // NOLINT(bugprone-use-after-move)
    EXPECT_EQ(b.size(), 64u);
    for (auto v : b.data()) {
        EXPECT_EQ(v, 0xAA);
    }
}

TEST(SecureBuffer, NoCopy) {
    static_assert(!std::is_copy_constructible_v<SecureBuffer>,
                  "SecureBuffer must not be copy-constructible");
    static_assert(!std::is_copy_assignable_v<SecureBuffer>,
                  "SecureBuffer must not be copy-assignable");
}

TEST(SecureBuffer, EmptyBuffer) {
    SecureBuffer buf(0);
    EXPECT_EQ(buf.size(), 0u);
    EXPECT_TRUE(buf.empty());
    EXPECT_TRUE(buf.data().empty());
}

}  // namespace iris::test
