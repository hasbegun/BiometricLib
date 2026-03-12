#include <iris/nodes/template_identity_filter.hpp>

#include <gtest/gtest.h>

using namespace iris;

// --- Helpers ---

/// Create AlignedTemplates with explicit distance matrix values.
/// `distances` is a flat upper-triangle: {d01, d02, ..., d12, d13, ...}
static AlignedTemplates make_aligned_with_distances(
    size_t n, const std::vector<double>& upper_triangle) {
    AlignedTemplates at;
    at.distances = DistanceMatrix(n);
    at.reference_template_id = "img0";

    size_t idx = 0;
    for (size_t i = 0; i < n; ++i) {
        IrisTemplateWithId tmpl;
        tmpl.image_id = "img" + std::to_string(i);
        PackedIrisCode ic;
        ic.code_bits.fill(0);
        ic.mask_bits.fill(~uint64_t{0});
        tmpl.iris_codes.push_back(ic);
        PackedIrisCode mc;
        mc.code_bits.fill(~uint64_t{0});
        mc.mask_bits.fill(~uint64_t{0});
        tmpl.mask_codes.push_back(mc);
        at.templates.push_back(std::move(tmpl));

        for (size_t j = i + 1; j < n; ++j) {
            at.distances(i, j) = upper_triangle[idx];
            at.distances(j, i) = upper_triangle[idx];
            ++idx;
        }
    }
    return at;
}

// --- Tests ---

TEST(TemplateIdentityFilter, AllSameIdentity) {
    TemplateIdentityFilter filter;

    // 3 templates, all close (distance = 0.1)
    auto input = make_aligned_with_distances(3, {0.1, 0.1, 0.1});

    auto result = filter.run(input);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().templates.size(), 3u);
}

TEST(TemplateIdentityFilter, OneOutlier_Remove) {
    TemplateIdentityFilter::Params p;
    p.identity_distance_threshold = 0.35;
    p.validation_action = IdentityValidationAction::Remove;
    TemplateIdentityFilter filter(p);

    // 3 templates: 0-1 close (0.1), 0-2 far (0.5), 1-2 far (0.5)
    // Component {0,1} is largest, template 2 is outlier
    auto input = make_aligned_with_distances(3, {0.1, 0.5, 0.5});

    auto result = filter.run(input);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().templates.size(), 2u);
    EXPECT_EQ(result.value().templates[0].image_id, "img0");
    EXPECT_EQ(result.value().templates[1].image_id, "img1");
}

TEST(TemplateIdentityFilter, OneOutlier_RaiseError) {
    TemplateIdentityFilter::Params p;
    p.identity_distance_threshold = 0.35;
    p.validation_action = IdentityValidationAction::RaiseError;
    TemplateIdentityFilter filter(p);

    auto input = make_aligned_with_distances(3, {0.1, 0.5, 0.5});

    auto result = filter.run(input);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::IdentityValidationFailed);
}

TEST(TemplateIdentityFilter, TwoClusters) {
    TemplateIdentityFilter::Params p;
    p.identity_distance_threshold = 0.35;
    p.validation_action = IdentityValidationAction::Remove;
    TemplateIdentityFilter filter(p);

    // 4 templates: {0,1} close (0.1), {2,3} close (0.1), others far (0.5)
    // d01=0.1, d02=0.5, d03=0.5, d12=0.5, d13=0.5, d23=0.1
    auto input = make_aligned_with_distances(4, {0.1, 0.5, 0.5, 0.5, 0.5, 0.1});

    auto result = filter.run(input);
    ASSERT_TRUE(result.has_value());
    // Both clusters have same size (2), first found wins → {0,1}
    EXPECT_EQ(result.value().templates.size(), 2u);
}

TEST(TemplateIdentityFilter, AllOutliers) {
    TemplateIdentityFilter::Params p;
    p.identity_distance_threshold = 0.01;  // Very strict
    p.validation_action = IdentityValidationAction::Remove;
    p.min_templates_after_validation = 1;
    TemplateIdentityFilter filter(p);

    // All far from each other
    auto input = make_aligned_with_distances(3, {0.5, 0.5, 0.5});

    auto result = filter.run(input);
    ASSERT_TRUE(result.has_value());
    // Each template is its own component, largest has size 1
    EXPECT_EQ(result.value().templates.size(), 1u);
}

TEST(TemplateIdentityFilter, MinTemplatesViolation) {
    TemplateIdentityFilter::Params p;
    p.identity_distance_threshold = 0.35;
    p.validation_action = IdentityValidationAction::Remove;
    p.min_templates_after_validation = 3;
    TemplateIdentityFilter filter(p);

    // Only 2 out of 3 are in the largest cluster
    auto input = make_aligned_with_distances(3, {0.1, 0.5, 0.5});

    auto result = filter.run(input);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::IdentityValidationFailed);
}

TEST(TemplateIdentityFilter, SingleTemplate) {
    TemplateIdentityFilter filter;
    auto input = make_aligned_with_distances(1, {});

    auto result = filter.run(input);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().templates.size(), 1u);
}

TEST(TemplateIdentityFilter, EmptyInput) {
    TemplateIdentityFilter filter;
    AlignedTemplates at;

    auto result = filter.run(at);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::IdentityValidationFailed);
}

TEST(TemplateIdentityFilter, DistanceMatrixPreserved) {
    TemplateIdentityFilter filter;
    auto input = make_aligned_with_distances(3, {0.1, 0.2, 0.15});

    auto result = filter.run(input);
    ASSERT_TRUE(result.has_value());

    // Distance matrix should be rebuilt for the kept indices
    const auto& dm = result.value().distances;
    EXPECT_EQ(dm.size, 3u);
    EXPECT_DOUBLE_EQ(dm(0, 1), 0.1);
    EXPECT_DOUBLE_EQ(dm(0, 2), 0.2);
    EXPECT_DOUBLE_EQ(dm(1, 2), 0.15);
}

TEST(TemplateIdentityFilter, NodeParams) {
    std::unordered_map<std::string, std::string> np;
    np["identity_distance_threshold"] = "0.30";
    np["validation_action"] = "raise_error";
    np["min_templates_after_validation"] = "2";

    TemplateIdentityFilter filter(np);
    EXPECT_NEAR(filter.params().identity_distance_threshold, 0.30, 1e-10);
    EXPECT_EQ(filter.params().validation_action, IdentityValidationAction::RaiseError);
    EXPECT_EQ(filter.params().min_templates_after_validation, 2u);
}
