#include <iris/nodes/iris_bbox_calculator.hpp>

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

TEST(IrisBBox, BasicCircle) {
    IRImage img;
    img.data = cv::Mat::zeros(400, 400, CV_8UC1);

    GeometryPolygons gp;
    gp.iris = make_circle(200.0f, 200.0f, 50.0f);

    IrisBBoxCalculator node;
    auto result = node.run(img, gp);
    ASSERT_TRUE(result.has_value());

    EXPECT_NEAR(result->x_min, 150.0, 1.0);
    EXPECT_NEAR(result->y_min, 150.0, 1.0);
    EXPECT_NEAR(result->x_max, 250.0, 1.0);
    EXPECT_NEAR(result->y_max, 250.0, 1.0);
}

TEST(IrisBBox, WithBuffer) {
    IRImage img;
    img.data = cv::Mat::zeros(400, 400, CV_8UC1);

    GeometryPolygons gp;
    gp.iris = make_circle(200.0f, 200.0f, 50.0f);

    IrisBBoxCalculator::Params params;
    params.buffer = 2.0;
    IrisBBoxCalculator node{params};
    auto result = node.run(img, gp);
    ASSERT_TRUE(result.has_value());

    // Width doubles: was ~100, pad 50 each side -> x_min ≈ 100, x_max ≈ 300
    EXPECT_LT(result->x_min, 110.0);
    EXPECT_GT(result->x_max, 290.0);
}

TEST(IrisBBox, ClampedToImage) {
    IRImage img;
    img.data = cv::Mat::zeros(200, 200, CV_8UC1);

    GeometryPolygons gp;
    gp.iris = make_circle(10.0f, 10.0f, 50.0f);

    IrisBBoxCalculator node;
    auto result = node.run(img, gp);
    ASSERT_TRUE(result.has_value());

    // Should be clamped to 0
    EXPECT_GE(result->x_min, 0.0);
    EXPECT_GE(result->y_min, 0.0);
}

TEST(IrisBBox, EmptyContourFails) {
    IRImage img;
    img.data = cv::Mat::zeros(100, 100, CV_8UC1);
    GeometryPolygons gp;

    IrisBBoxCalculator node;
    auto result = node.run(img, gp);
    EXPECT_FALSE(result.has_value());
}
