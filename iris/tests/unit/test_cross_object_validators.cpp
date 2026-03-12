#include <iris/nodes/cross_object_validators.hpp>

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

// ===========================================================================
// EyeCentersInsideImageValidator
// ===========================================================================

TEST(EyeCentersInsideImageValidator, CentersWellInside) {
    IRImage img;
    img.data = cv::Mat(200, 300, CV_8UC1, cv::Scalar(128));

    EyeCenters ec;
    ec.pupil_center = {150.0, 100.0};
    ec.iris_center = {150.0, 100.0};

    EyeCentersInsideImageValidator::Params params;
    params.min_distance_to_border = 20.0;
    EyeCentersInsideImageValidator node{params};
    EXPECT_TRUE(node.run(img, ec).has_value());
}

TEST(EyeCentersInsideImageValidator, CenterTooCloseToLeft) {
    IRImage img;
    img.data = cv::Mat(200, 300, CV_8UC1, cv::Scalar(128));

    EyeCenters ec;
    ec.pupil_center = {5.0, 100.0};
    ec.iris_center = {150.0, 100.0};

    EyeCentersInsideImageValidator::Params params;
    params.min_distance_to_border = 20.0;
    EyeCentersInsideImageValidator node{params};
    EXPECT_FALSE(node.run(img, ec).has_value());
}

TEST(EyeCentersInsideImageValidator, IrisCenterTooCloseToTop) {
    IRImage img;
    img.data = cv::Mat(200, 300, CV_8UC1, cv::Scalar(128));

    EyeCenters ec;
    ec.pupil_center = {150.0, 100.0};
    ec.iris_center = {150.0, 3.0};

    EyeCentersInsideImageValidator::Params params;
    params.min_distance_to_border = 10.0;
    EyeCentersInsideImageValidator node{params};
    EXPECT_FALSE(node.run(img, ec).has_value());
}

TEST(EyeCentersInsideImageValidator, ZeroDistanceAlwaysPasses) {
    IRImage img;
    img.data = cv::Mat(200, 300, CV_8UC1, cv::Scalar(128));

    EyeCenters ec;
    ec.pupil_center = {0.5, 0.5};
    ec.iris_center = {299.5, 199.5};

    EyeCentersInsideImageValidator node;  // default min_distance=0
    EXPECT_TRUE(node.run(img, ec).has_value());
}

TEST(EyeCentersInsideImageValidator, NodeParamsConstruction) {
    std::unordered_map<std::string, std::string> np;
    np["min_distance_to_border"] = "15.0";
    EyeCentersInsideImageValidator node{np};

    IRImage img;
    img.data = cv::Mat(200, 300, CV_8UC1, cv::Scalar(128));

    EyeCenters ec;
    ec.pupil_center = {150.0, 100.0};
    ec.iris_center = {150.0, 100.0};
    EXPECT_TRUE(node.run(img, ec).has_value());
}

// ===========================================================================
// ExtrapolatedPolygonsInsideImageValidator
// ===========================================================================

TEST(ExtrapolatedPolygonsInsideImageValidator, AllInside) {
    IRImage img;
    img.data = cv::Mat(400, 400, CV_8UC1, cv::Scalar(128));

    GeometryPolygons gp;
    gp.pupil = make_circle(200.0f, 200.0f, 30.0f);
    gp.iris = make_circle(200.0f, 200.0f, 80.0f);
    gp.eyeball = make_circle(200.0f, 200.0f, 150.0f);

    ExtrapolatedPolygonsInsideImageValidator::Params params;
    params.min_pupil_allowed_percentage = 0.9;
    params.min_iris_allowed_percentage = 0.9;
    params.min_eyeball_allowed_percentage = 0.9;
    ExtrapolatedPolygonsInsideImageValidator node{params};
    EXPECT_TRUE(node.run(img, gp).has_value());
}

TEST(ExtrapolatedPolygonsInsideImageValidator, EyeballPartiallyOutside) {
    IRImage img;
    img.data = cv::Mat(200, 200, CV_8UC1, cv::Scalar(128));

    GeometryPolygons gp;
    gp.pupil = make_circle(100.0f, 100.0f, 20.0f);
    gp.iris = make_circle(100.0f, 100.0f, 50.0f);
    gp.eyeball = make_circle(100.0f, 100.0f, 150.0f);  // extends far outside

    ExtrapolatedPolygonsInsideImageValidator::Params params;
    params.min_eyeball_allowed_percentage = 0.95;
    ExtrapolatedPolygonsInsideImageValidator node{params};
    EXPECT_FALSE(node.run(img, gp).has_value());
}

TEST(ExtrapolatedPolygonsInsideImageValidator, ZeroThresholdAlwaysPasses) {
    IRImage img;
    img.data = cv::Mat(50, 50, CV_8UC1, cv::Scalar(128));

    GeometryPolygons gp;
    gp.pupil = make_circle(25.0f, 25.0f, 30.0f);  // extends outside
    gp.iris = make_circle(25.0f, 25.0f, 40.0f);
    gp.eyeball = make_circle(25.0f, 25.0f, 60.0f);

    ExtrapolatedPolygonsInsideImageValidator node;  // all thresholds 0
    EXPECT_TRUE(node.run(img, gp).has_value());
}

TEST(ExtrapolatedPolygonsInsideImageValidator, NodeParamsConstruction) {
    std::unordered_map<std::string, std::string> np;
    np["min_pupil_allowed_percentage"] = "0.5";
    np["min_iris_allowed_percentage"] = "0.5";
    np["min_eyeball_allowed_percentage"] = "0.5";
    ExtrapolatedPolygonsInsideImageValidator node{np};

    IRImage img;
    img.data = cv::Mat(400, 400, CV_8UC1, cv::Scalar(128));

    GeometryPolygons gp;
    gp.pupil = make_circle(200.0f, 200.0f, 30.0f);
    gp.iris = make_circle(200.0f, 200.0f, 80.0f);
    gp.eyeball = make_circle(200.0f, 200.0f, 150.0f);
    EXPECT_TRUE(node.run(img, gp).has_value());
}
