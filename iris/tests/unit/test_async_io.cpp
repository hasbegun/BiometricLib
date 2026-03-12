#include <iris/core/async_io.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

#include <gtest/gtest.h>

using namespace iris;

class AsyncIOTest : public ::testing::Test {
protected:
    std::filesystem::path temp_dir_;

    void SetUp() override {
        temp_dir_ = std::filesystem::temp_directory_path() / "iris_test_async_io";
        std::filesystem::create_directories(temp_dir_);
    }

    void TearDown() override { std::filesystem::remove_all(temp_dir_); }
};

TEST_F(AsyncIOTest, WriteAndReadFile) {
    auto path = temp_dir_ / "test.bin";
    std::vector<uint8_t> data = {1, 2, 3, 4, 5, 42, 255};

    auto write_result = AsyncIO::write_file(path, data).get();
    ASSERT_TRUE(write_result.has_value()) << write_result.error().message;

    auto read_result = AsyncIO::read_file(path).get();
    ASSERT_TRUE(read_result.has_value()) << read_result.error().message;
    EXPECT_EQ(read_result.value(), data);
}

TEST_F(AsyncIOTest, ReadNonexistentFails) {
    auto result = AsyncIO::read_file(temp_dir_ / "does_not_exist.bin").get();
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::IoFailed);
}

TEST_F(AsyncIOTest, ConcurrentReads) {
    // Write a file, then read it concurrently
    auto path = temp_dir_ / "concurrent.bin";
    std::vector<uint8_t> data(1000, 0xAB);
    AsyncIO::write_file(path, data).get();

    std::vector<std::future<Result<std::vector<uint8_t>>>> futures;
    for (int i = 0; i < 10; ++i) {
        futures.push_back(AsyncIO::read_file(path));
    }

    for (auto& f : futures) {
        auto result = f.get();
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), data);
    }
}

TEST_F(AsyncIOTest, LoadYaml) {
    auto path = temp_dir_ / "test.yaml";
    std::string yaml_content = "key: value\nlist:\n  - item1\n  - item2\n";
    {
        std::ofstream f(path);
        f << yaml_content;
    }

    auto result = AsyncIO::load_yaml(path).get();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), yaml_content);
}

TEST_F(AsyncIOTest, DownloadModelNoNetwork) {
    // Without actual HTTP, this should fail gracefully
    auto result =
        AsyncIO::download_model("https://example.com/model.onnx", temp_dir_).get();
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::IoFailed);
}
