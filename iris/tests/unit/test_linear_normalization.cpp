#include <iris/nodes/linear_normalization.hpp>

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

TEST(LinearNormalization, ConcentricCircles) {
    IRImage img;
    img.data = cv::Mat(200, 200, CV_8UC1, cv::Scalar(128));

    GeometryPolygons gp;
    gp.pupil = make_circle(100.0f, 100.0f, 20.0f);
    gp.iris = make_circle(100.0f, 100.0f, 50.0f);
    gp.eyeball = make_circle(100.0f, 100.0f, 80.0f);

    NoiseMask nm{.mask = cv::Mat::zeros(200, 200, CV_8UC1)};
    EyeOrientation orient{.angle = 0.0};

    LinearNormalization node;
    auto result = node.run(img, nm, gp, orient);
    ASSERT_TRUE(result.has_value());

    // Output should be (res_in_r=128, num_points=360)
    EXPECT_EQ(result->normalized_image.rows, 128);
    EXPECT_EQ(result->normalized_image.cols, 360);
    EXPECT_EQ(result->normalized_image.type(), CV_64FC1);
}

TEST(LinearNormalization, OversatMasking) {
    IRImage img;
    img.data = cv::Mat(200, 200, CV_8UC1, cv::Scalar(255));

    GeometryPolygons gp;
    gp.pupil = make_circle(100.0f, 100.0f, 20.0f);
    gp.iris = make_circle(100.0f, 100.0f, 50.0f);
    gp.eyeball = make_circle(100.0f, 100.0f, 80.0f);

    NoiseMask nm{.mask = cv::Mat::zeros(200, 200, CV_8UC1)};
    EyeOrientation orient{.angle = 0.0};

    LinearNormalization::Params params;
    params.oversat_threshold = 254;
    LinearNormalization node{params};
    auto result = node.run(img, nm, gp, orient);
    ASSERT_TRUE(result.has_value());

    // All pixels oversaturated -> mask should be mostly 0
    EXPECT_EQ(cv::countNonZero(result->normalized_mask), 0);
}

TEST(LinearNormalization, MismatchedPointsFails) {
    IRImage img;
    img.data = cv::Mat(100, 100, CV_8UC1, cv::Scalar(100));

    GeometryPolygons gp;
    gp.pupil = make_circle(50.0f, 50.0f, 10.0f, 100);
    gp.iris = make_circle(50.0f, 50.0f, 30.0f, 200);
    gp.eyeball = make_circle(50.0f, 50.0f, 45.0f);

    NoiseMask nm{.mask = cv::Mat::zeros(100, 100, CV_8UC1)};
    EyeOrientation orient{.angle = 0.0};

    LinearNormalization node;
    auto result = node.run(img, nm, gp, orient);
    EXPECT_FALSE(result.has_value());
}

TEST(LinearNormalization, NodeParamsConstruction) {
    std::unordered_map<std::string, std::string> np;
    np["res_in_r"] = "64";
    np["oversat_threshold"] = "250";

    LinearNormalization node{np};

    IRImage img;
    img.data = cv::Mat(200, 200, CV_8UC1, cv::Scalar(128));

    GeometryPolygons gp;
    gp.pupil = make_circle(100.0f, 100.0f, 20.0f);
    gp.iris = make_circle(100.0f, 100.0f, 50.0f);
    gp.eyeball = make_circle(100.0f, 100.0f, 80.0f);

    NoiseMask nm{.mask = cv::Mat::zeros(200, 200, CV_8UC1)};
    EyeOrientation orient{.angle = 0.0};

    auto result = node.run(img, nm, gp, orient);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->normalized_image.rows, 64);
}
