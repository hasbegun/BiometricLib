#include <iris/nodes/perspective_normalization.hpp>

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

TEST(PerspectiveNormalization, ConcentricCircles) {
    IRImage img;
    img.data = cv::Mat(200, 200, CV_8UC1, cv::Scalar(128));

    GeometryPolygons gp;
    gp.pupil = make_circle(100.0f, 100.0f, 20.0f);
    gp.iris = make_circle(100.0f, 100.0f, 50.0f);
    gp.eyeball = make_circle(100.0f, 100.0f, 80.0f);

    NoiseMask nm{.mask = cv::Mat::zeros(200, 200, CV_8UC1)};
    EyeOrientation orient{.angle = 0.0};

    PerspectiveNormalization::Params params;
    params.res_in_phi = 256;
    params.res_in_r = 64;
    params.skip_boundary_points = 30;
    params.intermediate_radiuses = {0.0, 0.5, 1.0};
    PerspectiveNormalization node{params};

    auto result = node.run(img, nm, gp, orient);
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(result->normalized_image.rows, 64);
    EXPECT_EQ(result->normalized_image.cols, 256);
    EXPECT_EQ(result->normalized_image.type(), CV_64FC1);
}

TEST(PerspectiveNormalization, ProducesNonZeroOutput) {
    IRImage img;
    img.data = cv::Mat(200, 200, CV_8UC1, cv::Scalar(128));

    GeometryPolygons gp;
    gp.pupil = make_circle(100.0f, 100.0f, 20.0f);
    gp.iris = make_circle(100.0f, 100.0f, 50.0f);
    gp.eyeball = make_circle(100.0f, 100.0f, 80.0f);

    NoiseMask nm{.mask = cv::Mat::zeros(200, 200, CV_8UC1)};
    EyeOrientation orient{.angle = 0.0};

    PerspectiveNormalization::Params params;
    params.res_in_phi = 128;
    params.res_in_r = 32;
    params.skip_boundary_points = 30;
    params.intermediate_radiuses = {0.0, 0.33, 0.66, 1.0};
    PerspectiveNormalization node{params};

    auto result = node.run(img, nm, gp, orient);
    ASSERT_TRUE(result.has_value());

    // Most of the normalized image should have nonzero values
    double min_val = 0.0;
    double max_val = 0.0;
    cv::minMaxLoc(result->normalized_image, &min_val, &max_val);
    EXPECT_GT(max_val, 0.0);
}

TEST(PerspectiveNormalization, MismatchedPointsFails) {
    IRImage img;
    img.data = cv::Mat(100, 100, CV_8UC1, cv::Scalar(100));

    GeometryPolygons gp;
    gp.pupil = make_circle(50.0f, 50.0f, 10.0f, 100);
    gp.iris = make_circle(50.0f, 50.0f, 30.0f, 200);
    gp.eyeball = make_circle(50.0f, 50.0f, 45.0f);

    NoiseMask nm{.mask = cv::Mat::zeros(100, 100, CV_8UC1)};
    EyeOrientation orient{.angle = 0.0};

    PerspectiveNormalization node;
    auto result = node.run(img, nm, gp, orient);
    EXPECT_FALSE(result.has_value());
}
