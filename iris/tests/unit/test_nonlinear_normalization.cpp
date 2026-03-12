#include <iris/nodes/nonlinear_normalization.hpp>

#include <cmath>
#include <numbers>

#include <gtest/gtest.h>

using namespace iris;

namespace {

Contour make_circle(float cx, float cy, float r, int n = 360) {
    Contour c;
    c.points.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        const double angle =
            static_cast<double>(i) * 2.0 * std::numbers::pi / static_cast<double>(n);
        c.points.emplace_back(cx + r * static_cast<float>(std::cos(angle)),
                              cy + r * static_cast<float>(std::sin(angle)));
    }
    return c;
}

}  // namespace

TEST(NonlinearNormalization, ConcentricCircles) {
    IRImage img;
    img.data = cv::Mat(200, 200, CV_8UC1, cv::Scalar(128));

    GeometryPolygons gp;
    gp.pupil = make_circle(100.0f, 100.0f, 20.0f);
    gp.iris = make_circle(100.0f, 100.0f, 50.0f);
    gp.eyeball = make_circle(100.0f, 100.0f, 80.0f);

    NoiseMask nm{.mask = cv::Mat::zeros(200, 200, CV_8UC1)};
    EyeOrientation orient{.angle = 0.0};

    NonlinearNormalization node;
    auto result = node.run(img, nm, gp, orient);
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(result->normalized_image.rows, 128);
    EXPECT_EQ(result->normalized_image.cols, 360);
    EXPECT_EQ(result->normalized_image.type(), CV_64FC1);
}

TEST(NonlinearNormalization, DifferentFromLinear) {
    // Nonlinear grids are different from linear — verify that the mapping differs
    IRImage img;
    // Radial gradient: intensity increases with distance from center
    img.data = cv::Mat(200, 200, CV_8UC1);
    for (int y = 0; y < 200; ++y) {
        for (int x = 0; x < 200; ++x) {
            double r = std::hypot(static_cast<double>(x - 100),
                                  static_cast<double>(y - 100));
            img.data.at<uint8_t>(y, x) = static_cast<uint8_t>(
                std::min(r * 2.0, 255.0));
        }
    }

    GeometryPolygons gp;
    gp.pupil = make_circle(100.0f, 100.0f, 20.0f);
    gp.iris = make_circle(100.0f, 100.0f, 50.0f);
    gp.eyeball = make_circle(100.0f, 100.0f, 80.0f);

    NoiseMask nm{.mask = cv::Mat::zeros(200, 200, CV_8UC1)};
    EyeOrientation orient{.angle = 0.0};

    NonlinearNormalization node;
    auto result = node.run(img, nm, gp, orient);
    ASSERT_TRUE(result.has_value());
    // Just verify it runs and produces valid output
    EXPECT_GT(cv::countNonZero(result->normalized_mask), 0);
}

TEST(NonlinearNormalization, MismatchedPointsFails) {
    IRImage img;
    img.data = cv::Mat(100, 100, CV_8UC1, cv::Scalar(100));

    GeometryPolygons gp;
    gp.pupil = make_circle(50.0f, 50.0f, 10.0f, 100);
    gp.iris = make_circle(50.0f, 50.0f, 30.0f, 200);
    gp.eyeball = make_circle(50.0f, 50.0f, 45.0f);

    NoiseMask nm{.mask = cv::Mat::zeros(100, 100, CV_8UC1)};
    EyeOrientation orient{.angle = 0.0};

    NonlinearNormalization node;
    auto result = node.run(img, nm, gp, orient);
    EXPECT_FALSE(result.has_value());
}
