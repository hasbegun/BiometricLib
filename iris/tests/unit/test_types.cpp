#include <iris/core/iris_code_packed.hpp>
#include <iris/core/types.hpp>

#include <cstring>
#include <random>

#include <gtest/gtest.h>

using namespace iris;

TEST(PackedIrisCode, ZeroInitialized) {
    PackedIrisCode code{};
    for (size_t i = 0; i < PackedIrisCode::kNumWords; ++i) {
        EXPECT_EQ(code.code_bits[i], 0u);
        EXPECT_EQ(code.mask_bits[i], 0u);
    }
}

TEST(PackedIrisCode, FromMatRoundTrip) {
    // Create a known pattern: alternating 0/1
    cv::Mat code(PackedIrisCode::kRows, PackedIrisCode::kBitsPerRow, CV_8UC1);
    cv::Mat mask(PackedIrisCode::kRows, PackedIrisCode::kBitsPerRow, CV_8UC1);

    for (int r = 0; r < code.rows; ++r) {
        for (int c = 0; c < code.cols; ++c) {
            code.at<uint8_t>(r, c) = static_cast<uint8_t>((r + c) % 2);
            mask.at<uint8_t>(r, c) = 1;  // all valid
        }
    }

    auto packed = PackedIrisCode::from_mat(code, mask);
    auto [code_out, mask_out] = packed.to_mat();

    ASSERT_EQ(code_out.rows, code.rows);
    ASSERT_EQ(code_out.cols, code.cols);

    for (int r = 0; r < code.rows; ++r) {
        for (int c = 0; c < code.cols; ++c) {
            EXPECT_EQ(code_out.at<uint8_t>(r, c), code.at<uint8_t>(r, c))
                << "Mismatch at (" << r << ", " << c << ")";
            EXPECT_EQ(mask_out.at<uint8_t>(r, c), mask.at<uint8_t>(r, c))
                << "Mask mismatch at (" << r << ", " << c << ")";
        }
    }
}

TEST(PackedIrisCode, FromMatRoundTripRandom) {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 1);

    cv::Mat code(PackedIrisCode::kRows, PackedIrisCode::kBitsPerRow, CV_8UC1);
    cv::Mat mask(PackedIrisCode::kRows, PackedIrisCode::kBitsPerRow, CV_8UC1);

    for (int r = 0; r < code.rows; ++r) {
        for (int c = 0; c < code.cols; ++c) {
            code.at<uint8_t>(r, c) = static_cast<uint8_t>(dist(rng));
            mask.at<uint8_t>(r, c) = static_cast<uint8_t>(dist(rng));
        }
    }

    auto packed = PackedIrisCode::from_mat(code, mask);
    auto [code_out, mask_out] = packed.to_mat();

    for (int r = 0; r < code.rows; ++r) {
        for (int c = 0; c < code.cols; ++c) {
            EXPECT_EQ(code_out.at<uint8_t>(r, c), code.at<uint8_t>(r, c));
            EXPECT_EQ(mask_out.at<uint8_t>(r, c), mask.at<uint8_t>(r, c));
        }
    }
}

TEST(PackedIrisCode, RotateZeroIsIdentity) {
    std::mt19937 rng(123);
    std::uniform_int_distribution<int> dist(0, 1);

    cv::Mat code(PackedIrisCode::kRows, PackedIrisCode::kBitsPerRow, CV_8UC1);
    cv::Mat mask(PackedIrisCode::kRows, PackedIrisCode::kBitsPerRow, CV_8UC1);
    for (int r = 0; r < code.rows; ++r) {
        for (int c = 0; c < code.cols; ++c) {
            code.at<uint8_t>(r, c) = static_cast<uint8_t>(dist(rng));
            mask.at<uint8_t>(r, c) = static_cast<uint8_t>(dist(rng));
        }
    }

    auto packed = PackedIrisCode::from_mat(code, mask);
    auto rotated = packed.rotate(0);

    EXPECT_EQ(packed.code_bits, rotated.code_bits);
    EXPECT_EQ(packed.mask_bits, rotated.mask_bits);
}

TEST(PackedIrisCode, RotateFullCircleIsIdentity) {
    std::mt19937 rng(456);
    std::uniform_int_distribution<int> dist(0, 1);

    cv::Mat code(PackedIrisCode::kRows, PackedIrisCode::kBitsPerRow, CV_8UC1);
    cv::Mat mask(PackedIrisCode::kRows, PackedIrisCode::kBitsPerRow, CV_8UC1);
    for (int r = 0; r < code.rows; ++r) {
        for (int c = 0; c < code.cols; ++c) {
            code.at<uint8_t>(r, c) = static_cast<uint8_t>(dist(rng));
            mask.at<uint8_t>(r, c) = static_cast<uint8_t>(dist(rng));
        }
    }

    auto packed = PackedIrisCode::from_mat(code, mask);
    // 256 columns = full circle
    auto rotated = packed.rotate(256);

    EXPECT_EQ(packed.code_bits, rotated.code_bits);
    EXPECT_EQ(packed.mask_bits, rotated.mask_bits);
}

TEST(PackedIrisCode, RotateAndBackIsIdentity) {
    std::mt19937 rng(789);
    std::uniform_int_distribution<int> dist(0, 1);

    cv::Mat code(PackedIrisCode::kRows, PackedIrisCode::kBitsPerRow, CV_8UC1);
    cv::Mat mask(PackedIrisCode::kRows, PackedIrisCode::kBitsPerRow, CV_8UC1);
    for (int r = 0; r < code.rows; ++r) {
        for (int c = 0; c < code.cols; ++c) {
            code.at<uint8_t>(r, c) = static_cast<uint8_t>(dist(rng));
            mask.at<uint8_t>(r, c) = static_cast<uint8_t>(dist(rng));
        }
    }

    auto packed = PackedIrisCode::from_mat(code, mask);
    auto rotated = packed.rotate(15).rotate(-15);

    EXPECT_EQ(packed.code_bits, rotated.code_bits);
    EXPECT_EQ(packed.mask_bits, rotated.mask_bits);
}

// ---- Basic type construction tests ----

TEST(IRImage, Constructs) {
    IRImage img;
    img.data = cv::Mat(240, 320, CV_8UC1, cv::Scalar(128));
    img.image_id = "test";
    img.eye_side = EyeSide::Right;
    EXPECT_EQ(img.data.rows, 240);
    EXPECT_EQ(img.image_id, "test");
}

TEST(IrisTemplate, DefaultConstructs) {
    IrisTemplate tmpl;
    EXPECT_TRUE(tmpl.iris_codes.empty());
    EXPECT_EQ(tmpl.iris_code_version, "v2.0");
}

TEST(DistanceMatrix, AccessAndSymmetry) {
    DistanceMatrix dm(3);
    dm(0, 1) = 0.42;
    EXPECT_DOUBLE_EQ(dm(0, 1), 0.42);
}
