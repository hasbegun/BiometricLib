#include <iris/nodes/object_validators.hpp>

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
// Pupil2IrisPropertyValidator
// ===========================================================================

TEST(Pupil2IrisPropertyValidator, PassesValidProperty) {
    PupilToIrisProperty prop;
    prop.diameter_ratio = 0.3;
    prop.center_distance_ratio = 0.05;

    Pupil2IrisPropertyValidator node;
    auto result = node.run(prop);
    EXPECT_TRUE(result.has_value());
}

TEST(Pupil2IrisPropertyValidator, FailsTooConstricted) {
    PupilToIrisProperty prop;
    prop.diameter_ratio = 0.00001;
    prop.center_distance_ratio = 0.0;

    Pupil2IrisPropertyValidator::Params params;
    params.min_allowed_diameter_ratio = 0.1;
    Pupil2IrisPropertyValidator node{params};
    auto result = node.run(prop);
    EXPECT_FALSE(result.has_value());
}

TEST(Pupil2IrisPropertyValidator, FailsTooDilated) {
    PupilToIrisProperty prop;
    prop.diameter_ratio = 0.95;
    prop.center_distance_ratio = 0.0;

    Pupil2IrisPropertyValidator::Params params;
    params.max_allowed_diameter_ratio = 0.8;
    Pupil2IrisPropertyValidator node{params};
    auto result = node.run(prop);
    EXPECT_FALSE(result.has_value());
}

TEST(Pupil2IrisPropertyValidator, FailsOffcenter) {
    PupilToIrisProperty prop;
    prop.diameter_ratio = 0.3;
    prop.center_distance_ratio = 0.99;

    Pupil2IrisPropertyValidator::Params params;
    params.max_allowed_center_dist_ratio = 0.5;
    Pupil2IrisPropertyValidator node{params};
    auto result = node.run(prop);
    EXPECT_FALSE(result.has_value());
}

TEST(Pupil2IrisPropertyValidator, NodeParamsConstruction) {
    std::unordered_map<std::string, std::string> np;
    np["min_allowed_diameter_ratio"] = "0.1";
    np["max_allowed_diameter_ratio"] = "0.8";
    np["max_allowed_center_dist_ratio"] = "0.5";
    Pupil2IrisPropertyValidator node{np};

    PupilToIrisProperty prop;
    prop.diameter_ratio = 0.3;
    prop.center_distance_ratio = 0.1;
    EXPECT_TRUE(node.run(prop).has_value());
}

// ===========================================================================
// OffgazeValidator
// ===========================================================================

TEST(OffgazeValidator, PassesLowScore) {
    Offgaze off{.score = 0.1};
    OffgazeValidator::Params params;
    params.max_allowed_offgaze = 0.5;
    OffgazeValidator node{params};
    EXPECT_TRUE(node.run(off).has_value());
}

TEST(OffgazeValidator, FailsHighScore) {
    Offgaze off{.score = 0.8};
    OffgazeValidator::Params params;
    params.max_allowed_offgaze = 0.5;
    OffgazeValidator node{params};
    EXPECT_FALSE(node.run(off).has_value());
}

// ===========================================================================
// OcclusionValidator
// ===========================================================================

TEST(OcclusionValidator, PassesSufficientFraction) {
    EyeOcclusion occ{.fraction = 0.8};
    OcclusionValidator::Params params;
    params.min_allowed_occlusion = 0.5;
    OcclusionValidator node{params};
    EXPECT_TRUE(node.run(occ).has_value());
}

TEST(OcclusionValidator, FailsLowFraction) {
    EyeOcclusion occ{.fraction = 0.2};
    OcclusionValidator::Params params;
    params.min_allowed_occlusion = 0.5;
    OcclusionValidator node{params};
    EXPECT_FALSE(node.run(occ).has_value());
}

// ===========================================================================
// IsPupilInsideIrisValidator
// ===========================================================================

TEST(IsPupilInsideIrisValidator, PupilInsideIris) {
    GeometryPolygons gp;
    gp.pupil = make_circle(100.0f, 100.0f, 20.0f);
    gp.iris = make_circle(100.0f, 100.0f, 50.0f);

    IsPupilInsideIrisValidator node;
    EXPECT_TRUE(node.run(gp).has_value());
}

TEST(IsPupilInsideIrisValidator, PupilPartiallyOutside) {
    GeometryPolygons gp;
    gp.pupil = make_circle(140.0f, 100.0f, 20.0f);
    gp.iris = make_circle(100.0f, 100.0f, 50.0f);

    IsPupilInsideIrisValidator node;
    EXPECT_FALSE(node.run(gp).has_value());
}

// ===========================================================================
// SharpnessValidator
// ===========================================================================

TEST(SharpnessValidator, PassesHighSharpness) {
    Sharpness s{.score = 5.0};
    SharpnessValidator::Params params;
    params.min_sharpness = 2.0;
    SharpnessValidator node{params};
    EXPECT_TRUE(node.run(s).has_value());
}

TEST(SharpnessValidator, FailsLowSharpness) {
    Sharpness s{.score = 0.5};
    SharpnessValidator::Params params;
    params.min_sharpness = 2.0;
    SharpnessValidator node{params};
    EXPECT_FALSE(node.run(s).has_value());
}

// ===========================================================================
// IsMaskTooSmallValidator
// ===========================================================================

TEST(IsMaskTooSmallValidator, PassesLargeMask) {
    IrisTemplate tmpl;
    PackedIrisCode mc;
    mc.mask_bits.fill(0xFFFFFFFFFFFFFFFF);  // all bits set
    tmpl.mask_codes.push_back(mc);

    IsMaskTooSmallValidator::Params params;
    params.min_maskcodes_size = 100;
    IsMaskTooSmallValidator node{params};
    EXPECT_TRUE(node.run(tmpl).has_value());
}

TEST(IsMaskTooSmallValidator, FailsSmallMask) {
    IrisTemplate tmpl;
    PackedIrisCode mc;
    mc.mask_bits.fill(0);  // no bits set
    tmpl.mask_codes.push_back(mc);

    IsMaskTooSmallValidator::Params params;
    params.min_maskcodes_size = 100;
    IsMaskTooSmallValidator node{params};
    EXPECT_FALSE(node.run(tmpl).has_value());
}

// ===========================================================================
// PolygonsLengthValidator
// ===========================================================================

TEST(PolygonsLengthValidator, PassesLongPolygons) {
    GeometryPolygons gp;
    gp.pupil = make_circle(100.0f, 100.0f, 30.0f);
    gp.iris = make_circle(100.0f, 100.0f, 80.0f);

    PolygonsLengthValidator node;
    EXPECT_TRUE(node.run(gp).has_value());
}

TEST(PolygonsLengthValidator, FailsShortIris) {
    GeometryPolygons gp;
    gp.pupil = make_circle(100.0f, 100.0f, 30.0f);
    gp.iris = make_circle(100.0f, 100.0f, 5.0f);  // very small

    PolygonsLengthValidator::Params params;
    params.min_iris_length = 150.0;
    params.min_pupil_length = 10.0;
    PolygonsLengthValidator node{params};
    EXPECT_FALSE(node.run(gp).has_value());
}

TEST(PolygonsLengthValidator, FailsShortPupil) {
    GeometryPolygons gp;
    gp.pupil = make_circle(100.0f, 100.0f, 2.0f);  // very small
    gp.iris = make_circle(100.0f, 100.0f, 80.0f);

    PolygonsLengthValidator::Params params;
    params.min_iris_length = 10.0;
    params.min_pupil_length = 75.0;
    PolygonsLengthValidator node{params};
    EXPECT_FALSE(node.run(gp).has_value());
}
