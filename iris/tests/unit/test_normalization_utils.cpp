#include <iris/utils/normalization_utils.hpp>
#include <iris/utils/geometry_utils.hpp>

#include <cmath>
#include <numbers>

#include <gtest/gtest.h>

using namespace iris;

// --- correct_orientation ---

TEST(CorrectOrientation, ZeroAngleNoChange) {
    Contour pupil, iris;
    for (int i = 0; i < 8; ++i) {
        pupil.points.emplace_back(static_cast<float>(i), 0.0f);
        iris.points.emplace_back(static_cast<float>(i * 2), 0.0f);
    }
    auto [rp, ri] = correct_orientation(pupil, iris, 0.0);
    ASSERT_EQ(rp.points.size(), 8u);
    for (int i = 0; i < 8; ++i) {
        EXPECT_FLOAT_EQ(rp.points[static_cast<size_t>(i)].x,
                        static_cast<float>(i));
    }
}

TEST(CorrectOrientation, HalfTurnRolls) {
    // 360 points, 180 degree rotation -> shift by 180
    Contour pupil;
    for (int i = 0; i < 360; ++i) {
        pupil.points.emplace_back(static_cast<float>(i), 0.0f);
    }
    Contour iris = pupil;
    // 180 degrees = pi radians -> num_rotations = -round(180 * 360 / 360) = -180
    // shift = -(-180) % 360 = 180
    auto [rp, ri] = correct_orientation(pupil, iris, std::numbers::pi);
    // First point should be what was at index 180
    EXPECT_FLOAT_EQ(rp.points[0].x, 180.0f);
}

TEST(CorrectOrientation, EmptyContours) {
    Contour empty;
    auto [rp, ri] = correct_orientation(empty, empty, 1.0);
    EXPECT_TRUE(rp.points.empty());
}

// --- generate_iris_mask ---

TEST(GenerateIrisMask, BasicCircles) {
    const int sz = 100;
    cv::Mat noise = cv::Mat::zeros(sz, sz, CV_8UC1);

    // Create iris and eyeball contours as circles
    Contour iris_c, eyeball_c;
    for (int i = 0; i < 360; ++i) {
        double angle = static_cast<double>(i) * std::numbers::pi / 180.0;
        iris_c.points.emplace_back(
            static_cast<float>(50.0 + 20.0 * std::cos(angle)),
            static_cast<float>(50.0 + 20.0 * std::sin(angle)));
        eyeball_c.points.emplace_back(
            static_cast<float>(50.0 + 40.0 * std::cos(angle)),
            static_cast<float>(50.0 + 40.0 * std::sin(angle)));
    }

    GeometryPolygons gp;
    gp.iris = iris_c;
    gp.eyeball = eyeball_c;

    auto mask = generate_iris_mask(gp, noise);
    EXPECT_EQ(mask.type(), CV_8UC1);
    // Center should be inside iris
    EXPECT_EQ(mask.at<uint8_t>(50, 50), 1);
    // Corner should be outside
    EXPECT_EQ(mask.at<uint8_t>(0, 0), 0);
}

TEST(GenerateIrisMask, NoiseRemovesPixels) {
    const int sz = 100;
    // Noise mask covering the center
    cv::Mat noise = cv::Mat::zeros(sz, sz, CV_8UC1);
    noise.at<uint8_t>(50, 50) = 1;

    Contour iris_c, eyeball_c;
    for (int i = 0; i < 360; ++i) {
        double angle = static_cast<double>(i) * std::numbers::pi / 180.0;
        iris_c.points.emplace_back(
            static_cast<float>(50.0 + 20.0 * std::cos(angle)),
            static_cast<float>(50.0 + 20.0 * std::sin(angle)));
        eyeball_c.points.emplace_back(
            static_cast<float>(50.0 + 40.0 * std::cos(angle)),
            static_cast<float>(50.0 + 40.0 * std::sin(angle)));
    }

    GeometryPolygons gp;
    gp.iris = iris_c;
    gp.eyeball = eyeball_c;

    auto mask = generate_iris_mask(gp, noise);
    // Center should be removed by noise
    EXPECT_EQ(mask.at<uint8_t>(50, 50), 0);
    // Other iris point should still be 1
    EXPECT_EQ(mask.at<uint8_t>(50, 40), 1);
}

// --- normalize_all ---

TEST(NormalizeAll, BasicMapping) {
    // 10x10 image with gradient
    cv::Mat image(10, 10, CV_8UC1);
    for (int r = 0; r < 10; ++r) {
        for (int c = 0; c < 10; ++c) {
            image.at<uint8_t>(r, c) = static_cast<uint8_t>(r * 10 + c);
        }
    }
    cv::Mat iris_mask = cv::Mat::ones(10, 10, CV_8UC1);

    // 2x3 normalized output mapping to known pixels
    std::vector<std::vector<cv::Point>> src = {
        {{0, 0}, {5, 5}, {9, 9}},
        {{1, 1}, {3, 3}, {-1, -1}},  // out of bounds → 0
    };

    auto [norm_img, norm_mask] = normalize_all(image, iris_mask, src);
    ASSERT_EQ(norm_img.rows, 2);
    ASSERT_EQ(norm_img.cols, 3);

    // image[0,0] = 0 -> 0.0
    EXPECT_NEAR(norm_img.at<double>(0, 0), 0.0, 1e-10);
    // image[5,5] = 55 -> 55.0 (uint8 value preserved as double)
    EXPECT_NEAR(norm_img.at<double>(0, 1), 55.0, 1e-10);
    // Out of bounds -> 0
    EXPECT_NEAR(norm_img.at<double>(1, 2), 0.0, 1e-10);
    EXPECT_EQ(norm_mask.at<uint8_t>(1, 2), 0);
}

// --- interpolate_pixel_intensity ---

TEST(InterpolatePixel, CenterOfPixel) {
    cv::Mat image(3, 3, CV_8UC1, cv::Scalar(0));
    image.at<uint8_t>(1, 1) = 100;
    double val = interpolate_pixel_intensity(image, 1.0, 1.0);
    // At integer coords, bilinear should return the pixel value
    EXPECT_NEAR(val, 100.0, 1.0);
}

TEST(InterpolatePixel, Midpoint) {
    cv::Mat image(2, 2, CV_8UC1);
    image.at<uint8_t>(0, 0) = 0;
    image.at<uint8_t>(0, 1) = 100;
    image.at<uint8_t>(1, 0) = 0;
    image.at<uint8_t>(1, 1) = 100;
    double val = interpolate_pixel_intensity(image, 0.5, 0.5);
    // Average of 0, 100, 0, 100 = 50
    EXPECT_NEAR(val, 50.0, 1.0);
}

// --- get_pixel_or_default ---

TEST(GetPixelOrDefault, InBounds) {
    cv::Mat image(5, 5, CV_8UC1, cv::Scalar(42));
    EXPECT_NEAR(get_pixel_or_default(image, 2, 2, -1.0), 42.0, 1e-10);
}

TEST(GetPixelOrDefault, OutOfBounds) {
    cv::Mat image(5, 5, CV_8UC1, cv::Scalar(42));
    EXPECT_NEAR(get_pixel_or_default(image, -1, 0, -1.0), -1.0, 1e-10);
    EXPECT_NEAR(get_pixel_or_default(image, 0, 5, -1.0), -1.0, 1e-10);
}
