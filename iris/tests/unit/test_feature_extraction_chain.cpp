#include <iris/nodes/conv_filter_bank.hpp>
#include <iris/nodes/encoder.hpp>
#include <iris/nodes/fragile_bit_refinement.hpp>

#include <cmath>

#include <gtest/gtest.h>

using namespace iris;

// --- Helpers ---

static NormalizedIris make_iris_image(int rows, int cols) {
    NormalizedIris ni;
    ni.normalized_image = cv::Mat(rows, cols, CV_64FC1);
    for (int r = 0; r < rows; ++r) {
        auto* row = ni.normalized_image.ptr<double>(r);
        for (int c = 0; c < cols; ++c) {
            // Realistic-ish iris texture: sum of sinusoidal patterns
            const double x = static_cast<double>(c);
            const double y = static_cast<double>(r);
            row[c] = 0.5 + 0.2 * std::sin(x * 0.15) * std::cos(y * 0.3)
                         + 0.1 * std::sin(x * 0.4 + y * 0.1);
        }
    }
    ni.normalized_mask = cv::Mat(rows, cols, CV_8UC1, cv::Scalar(1));
    return ni;
}

// --- Integration tests ---

TEST(FeatureExtractionChain, NormalizedIrisToTemplate) {
    // Full chain: NormalizedIris → ConvFilterBank → FragileBitRefinement → Encoder
    GaborFilter::Params fp;
    fp.kernel_size_phi = 9;
    fp.kernel_size_rho = 9;
    fp.to_fixpoints = true;

    RegularProbeSchema::Params sp;
    sp.n_rows = 16;
    sp.n_cols = 256;

    ConvFilterBank bank(
        ConvFilterBank::Params{},
        {GaborFilter{fp}, GaborFilter{fp}},
        {RegularProbeSchema{sp}, RegularProbeSchema{sp}});

    FragileBitRefinement fbr;
    IrisEncoder encoder;

    auto input = make_iris_image(64, 512);

    auto filter_result = bank.run(input);
    ASSERT_TRUE(filter_result.has_value())
        << filter_result.error().message;

    auto refined_result = fbr.run(filter_result.value());
    ASSERT_TRUE(refined_result.has_value())
        << refined_result.error().message;

    auto template_result = encoder.run(refined_result.value());
    ASSERT_TRUE(template_result.has_value())
        << template_result.error().message;

    const auto& tmpl = template_result.value();
    EXPECT_EQ(tmpl.iris_codes.size(), 2u);
    EXPECT_EQ(tmpl.mask_codes.size(), 2u);
}

TEST(FeatureExtractionChain, OutputDimensions) {
    GaborFilter::Params fp;
    fp.kernel_size_phi = 9;
    fp.kernel_size_rho = 9;
    fp.to_fixpoints = true;

    RegularProbeSchema::Params sp;
    sp.n_rows = 16;
    sp.n_cols = 256;

    ConvFilterBank bank(
        ConvFilterBank::Params{},
        {GaborFilter{fp}},
        {RegularProbeSchema{sp}});

    FragileBitRefinement fbr;
    IrisEncoder encoder;

    auto input = make_iris_image(64, 512);
    auto fr = bank.run(input);
    ASSERT_TRUE(fr.has_value());

    auto refined = fbr.run(fr.value());
    ASSERT_TRUE(refined.has_value());

    auto tmpl = encoder.run(refined.value());
    ASSERT_TRUE(tmpl.has_value());

    // 1 filter → 1 packed iris code
    // Each position has 2 bits (real > 0, imag > 0) → 16 * 256 * 2 = 8192 bits
    EXPECT_EQ(tmpl.value().iris_codes.size(), 1u);
    EXPECT_EQ(PackedIrisCode::kTotalBits, 16u * 256u * 2u);
}

TEST(FeatureExtractionChain, ReproducibleOutput) {
    GaborFilter::Params fp;
    fp.kernel_size_phi = 9;
    fp.kernel_size_rho = 9;
    fp.to_fixpoints = true;

    RegularProbeSchema::Params sp;
    sp.n_rows = 16;
    sp.n_cols = 256;

    ConvFilterBank bank(
        ConvFilterBank::Params{},
        {GaborFilter{fp}},
        {RegularProbeSchema{sp}});

    FragileBitRefinement fbr;
    IrisEncoder encoder;

    auto input = make_iris_image(64, 512);

    // Run twice
    auto r1 = encoder.run(fbr.run(bank.run(input).value()).value());
    auto r2 = encoder.run(fbr.run(bank.run(input).value()).value());
    ASSERT_TRUE(r1.has_value() && r2.has_value());

    // Packed codes should be identical
    EXPECT_EQ(r1.value().iris_codes[0].code_bits, r2.value().iris_codes[0].code_bits);
    EXPECT_EQ(r1.value().mask_codes[0].code_bits, r2.value().mask_codes[0].code_bits);
}

TEST(FeatureExtractionChain, AllBlackImage) {
    GaborFilter::Params fp;
    fp.kernel_size_phi = 9;
    fp.kernel_size_rho = 9;
    fp.to_fixpoints = true;

    RegularProbeSchema::Params sp;
    sp.n_rows = 16;
    sp.n_cols = 256;

    ConvFilterBank bank(
        ConvFilterBank::Params{},
        {GaborFilter{fp}},
        {RegularProbeSchema{sp}});

    FragileBitRefinement fbr;
    IrisEncoder encoder;

    // Uniform black image
    NormalizedIris input;
    input.normalized_image = cv::Mat::zeros(64, 512, CV_64FC1);
    input.normalized_mask = cv::Mat(64, 512, CV_8UC1, cv::Scalar(1));

    auto fr = bank.run(input);
    ASSERT_TRUE(fr.has_value());

    auto refined = fbr.run(fr.value());
    ASSERT_TRUE(refined.has_value());

    auto tmpl = encoder.run(refined.value());
    ASSERT_TRUE(tmpl.has_value());

    // Should produce a valid template (even if mostly masked out)
    EXPECT_EQ(tmpl.value().iris_codes.size(), 1u);
}
