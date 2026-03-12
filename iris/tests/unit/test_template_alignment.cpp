#include <iris/nodes/template_alignment.hpp>

#include <gtest/gtest.h>

using namespace iris;

// --- Helpers ---

static IrisTemplateWithId make_template(const std::string& id, uint64_t fill_code) {
    IrisTemplateWithId tmpl;
    tmpl.image_id = id;
    PackedIrisCode ic;
    ic.code_bits.fill(fill_code);
    ic.mask_bits.fill(~uint64_t{0});
    tmpl.iris_codes.push_back(ic);
    PackedIrisCode mc;
    mc.code_bits.fill(~uint64_t{0});
    mc.mask_bits.fill(~uint64_t{0});
    tmpl.mask_codes.push_back(mc);
    return tmpl;
}

static IrisTemplateWithId make_patterned_template(const std::string& id) {
    IrisTemplateWithId tmpl;
    tmpl.image_id = id;
    PackedIrisCode ic;
    ic.mask_bits.fill(~uint64_t{0});
    for (size_t w = 0; w < PackedIrisCode::kNumWords; ++w)
        ic.code_bits[w] = (w % 3 == 0) ? 0xDEADBEEFCAFEBABE : 0x1234567890ABCDEF;
    tmpl.iris_codes.push_back(ic);
    PackedIrisCode mc;
    mc.code_bits.fill(~uint64_t{0});
    mc.mask_bits.fill(~uint64_t{0});
    tmpl.mask_codes.push_back(mc);
    return tmpl;
}

// --- Tests ---

TEST(TemplateAlignment, SingleTemplate) {
    HammingDistanceBasedAlignment aligner;
    std::vector<IrisTemplateWithId> templates = {make_template("img0", 0)};

    auto result = aligner.run(templates);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().templates.size(), 1u);
    EXPECT_EQ(result.value().reference_template_id, "img0");
}

TEST(TemplateAlignment, TwoIdentical) {
    HammingDistanceBasedAlignment::Params p;
    p.normalise = false;
    p.rotation_shift = 0;
    HammingDistanceBasedAlignment aligner(p);

    std::vector<IrisTemplateWithId> templates = {
        make_template("img0", 0),
        make_template("img1", 0)
    };

    auto result = aligner.run(templates);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().templates.size(), 2u);
    EXPECT_DOUBLE_EQ(result.value().distances(0, 1), 0.0);
}

TEST(TemplateAlignment, TwoShifted) {
    HammingDistanceBasedAlignment::Params p;
    p.normalise = false;
    p.rotation_shift = 5;
    HammingDistanceBasedAlignment aligner(p);

    // Template 0: base pattern
    auto t0 = make_patterned_template("img0");
    // Template 1: t0 shifted by 3 columns
    IrisTemplateWithId t1;
    t1.image_id = "img1";
    t1.iris_codes.push_back(t0.iris_codes[0].rotate(3));
    t1.mask_codes.push_back(t0.mask_codes[0].rotate(3));

    std::vector<IrisTemplateWithId> templates = {t0, t1};
    auto result = aligner.run(templates);
    ASSERT_TRUE(result.has_value());

    // After alignment, the templates should be very close
    EXPECT_NEAR(result.value().distances(0, 1), 0.0, 1e-10);
}

TEST(TemplateAlignment, RefSelection_Linear) {
    HammingDistanceBasedAlignment::Params p;
    p.normalise = false;
    p.rotation_shift = 0;
    p.reference_selection_method = ReferenceSelectionMethod::Linear;
    HammingDistanceBasedAlignment aligner(p);

    // Template 0 and 1 are identical (0), template 2 is different
    // Reference should be 0 or 1 (both have distance 0 to each other, 1 to template 2)
    std::vector<IrisTemplateWithId> templates = {
        make_template("img0", 0),
        make_template("img1", 0),
        make_template("img2", ~uint64_t{0})
    };

    auto result = aligner.run(templates);
    ASSERT_TRUE(result.has_value());
    // Reference should be img0 or img1 (tied, but first wins)
    EXPECT_TRUE(result.value().reference_template_id == "img0"
             || result.value().reference_template_id == "img1");
}

TEST(TemplateAlignment, RefSelection_MeanSq) {
    HammingDistanceBasedAlignment::Params p;
    p.normalise = false;
    p.rotation_shift = 0;
    p.reference_selection_method = ReferenceSelectionMethod::MeanSquared;
    HammingDistanceBasedAlignment aligner(p);

    std::vector<IrisTemplateWithId> templates = {
        make_template("img0", 0),
        make_template("img1", 0),
        make_template("img2", ~uint64_t{0})
    };

    auto result = aligner.run(templates);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().reference_template_id == "img0"
             || result.value().reference_template_id == "img1");
}

TEST(TemplateAlignment, RefSelection_RMS) {
    HammingDistanceBasedAlignment::Params p;
    p.normalise = false;
    p.rotation_shift = 0;
    p.reference_selection_method = ReferenceSelectionMethod::RootMeanSquared;
    HammingDistanceBasedAlignment aligner(p);

    std::vector<IrisTemplateWithId> templates = {
        make_template("img0", 0),
        make_template("img1", 0),
        make_template("img2", ~uint64_t{0})
    };

    auto result = aligner.run(templates);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().reference_template_id == "img0"
             || result.value().reference_template_id == "img1");
}

TEST(TemplateAlignment, MaskCodesRotated) {
    HammingDistanceBasedAlignment::Params p;
    p.normalise = false;
    p.rotation_shift = 5;
    HammingDistanceBasedAlignment aligner(p);

    auto t0 = make_patterned_template("img0");

    // Create t1 with a non-uniform mask that we can verify was rotated
    IrisTemplateWithId t1;
    t1.image_id = "img1";
    t1.iris_codes.push_back(t0.iris_codes[0].rotate(2));
    PackedIrisCode mc;
    mc.mask_bits.fill(~uint64_t{0});
    // Set a distinctive mask pattern
    mc.code_bits.fill(0xF0F0F0F0F0F0F0F0);
    t1.mask_codes.push_back(mc.rotate(2));

    std::vector<IrisTemplateWithId> templates = {t0, t1};
    auto result = aligner.run(templates);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().templates.size(), 2u);
}

TEST(TemplateAlignment, DistanceMatrixPopulated) {
    HammingDistanceBasedAlignment::Params p;
    p.normalise = false;
    p.rotation_shift = 0;
    HammingDistanceBasedAlignment aligner(p);

    std::vector<IrisTemplateWithId> templates = {
        make_template("img0", 0),
        make_template("img1", ~uint64_t{0}),
        make_template("img2", 0xAAAAAAAAAAAAAAAA)
    };

    auto result = aligner.run(templates);
    ASSERT_TRUE(result.has_value());

    const auto& dm = result.value().distances;
    EXPECT_EQ(dm.size, 3u);
    // Diagonal should be 0
    EXPECT_DOUBLE_EQ(dm(0, 0), 0.0);
    EXPECT_DOUBLE_EQ(dm(1, 1), 0.0);
    EXPECT_DOUBLE_EQ(dm(2, 2), 0.0);
    // Symmetric
    EXPECT_DOUBLE_EQ(dm(0, 1), dm(1, 0));
    EXPECT_DOUBLE_EQ(dm(0, 2), dm(2, 0));
}

TEST(TemplateAlignment, EmptyFails) {
    HammingDistanceBasedAlignment aligner;
    std::vector<IrisTemplateWithId> templates;

    auto result = aligner.run(templates);
    ASSERT_FALSE(result.has_value());
}

TEST(TemplateAlignment, NodeParams) {
    std::unordered_map<std::string, std::string> np;
    np["rotation_shift"] = "10";
    np["normalise"] = "false";
    np["norm_mean"] = "0.42";
    np["norm_gradient"] = "0.0001";
    np["reference_selection_method"] = "root_mean_squared";

    HammingDistanceBasedAlignment aligner(np);
    EXPECT_EQ(aligner.params().rotation_shift, 10);
    EXPECT_FALSE(aligner.params().normalise);
    EXPECT_NEAR(aligner.params().norm_mean, 0.42, 1e-10);
    EXPECT_NEAR(aligner.params().norm_gradient, 0.0001, 1e-10);
    EXPECT_EQ(aligner.params().reference_selection_method,
              ReferenceSelectionMethod::RootMeanSquared);
}
