#include "../comparison/npy_reader.hpp"
#include "../comparison/npy_writer.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>

#include <opencv2/core.hpp>

#include <gtest/gtest.h>

namespace iris::test {

namespace {
/// Create a temporary file path that is cleaned up on destruction.
struct TmpFile {
    std::filesystem::path path;
    explicit TmpFile(const std::string& name)
        : path(std::filesystem::temp_directory_path() / name) {}
    ~TmpFile() { std::filesystem::remove(path); }
};
}  // namespace

TEST(NpyWriter, Float32RoundTrip) {
    TmpFile tmp("test_float32.npy");

    cv::Mat orig(4, 8, CV_32FC1);
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 8; ++c) {
            orig.at<float>(r, c) = static_cast<float>(r * 8 + c) * 0.5f;
        }
    }

    npy::write_npy(tmp.path, orig);
    auto loaded = npy::read_npy(tmp.path);

    ASSERT_EQ(loaded.shape.size(), 2u);
    EXPECT_EQ(loaded.shape[0], 4u);
    EXPECT_EQ(loaded.shape[1], 8u);
    EXPECT_EQ(loaded.dtype, 'f');
    EXPECT_EQ(loaded.element_size, 4u);

    cv::Mat read_mat = loaded.to_mat();
    ASSERT_EQ(read_mat.rows, 4);
    ASSERT_EQ(read_mat.cols, 8);

    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 8; ++c) {
            EXPECT_FLOAT_EQ(read_mat.at<float>(r, c), orig.at<float>(r, c))
                << "at (" << r << ", " << c << ")";
        }
    }
}

TEST(NpyWriter, Uint8RoundTrip) {
    TmpFile tmp("test_uint8.npy");

    cv::Mat orig(3, 5, CV_8UC1);
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 5; ++c) {
            orig.at<uint8_t>(r, c) = static_cast<uint8_t>((r * 5 + c) * 17);
        }
    }

    npy::write_npy(tmp.path, orig);
    auto loaded = npy::read_npy(tmp.path);

    ASSERT_EQ(loaded.shape.size(), 2u);
    EXPECT_EQ(loaded.shape[0], 3u);
    EXPECT_EQ(loaded.shape[1], 5u);
    EXPECT_EQ(loaded.dtype, 'u');
    EXPECT_EQ(loaded.element_size, 1u);

    cv::Mat read_mat = loaded.to_mat();
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 5; ++c) {
            EXPECT_EQ(read_mat.at<uint8_t>(r, c), orig.at<uint8_t>(r, c));
        }
    }
}

TEST(NpyWriter, ThreeDShapeRoundTrip) {
    TmpFile tmp("test_3d.npy");

    // 2x3x4 float32 — flatten last dim into cols for cv::Mat
    std::vector<size_t> shape = {2, 3, 4};
    cv::Mat orig(2, 12, CV_32FC1);  // 3*4 = 12 cols
    for (int i = 0; i < 2 * 12; ++i) {
        orig.at<float>(i / 12, i % 12) = static_cast<float>(i);
    }

    npy::write_npy(tmp.path, orig, shape);
    auto loaded = npy::read_npy(tmp.path);

    ASSERT_EQ(loaded.shape.size(), 3u);
    EXPECT_EQ(loaded.shape[0], 2u);
    EXPECT_EQ(loaded.shape[1], 3u);
    EXPECT_EQ(loaded.shape[2], 4u);

    // to_mat folds 3D → 2D by multiplying last two dims
    cv::Mat read_mat = loaded.to_mat();
    EXPECT_EQ(read_mat.rows, 2);
    EXPECT_EQ(read_mat.cols, 12);  // 3 * 4

    for (int i = 0; i < 2 * 12; ++i) {
        EXPECT_FLOAT_EQ(read_mat.at<float>(i / 12, i % 12),
                         orig.at<float>(i / 12, i % 12));
    }
}

TEST(NpyWriter, WriteReadWriteIdenticalBytes) {
    TmpFile tmp1("test_rr1.npy");
    TmpFile tmp2("test_rr2.npy");

    cv::Mat orig(8, 16, CV_32FC1);
    for (int i = 0; i < 8 * 16; ++i) {
        orig.at<float>(i / 16, i % 16) = static_cast<float>(i) * 1.5f;
    }

    // Write → read → write again
    npy::write_npy(tmp1.path, orig);
    auto loaded = npy::read_npy(tmp1.path);
    cv::Mat mat2 = loaded.to_mat();
    npy::write_npy(tmp2.path, mat2);

    // Compare file bytes
    auto read_file = [](const std::filesystem::path& p) {
        std::ifstream f(p, std::ios::binary);
        return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),
                                     std::istreambuf_iterator<char>());
    };

    auto bytes1 = read_file(tmp1.path);
    auto bytes2 = read_file(tmp2.path);
    EXPECT_EQ(bytes1, bytes2);
}

}  // namespace iris::test
