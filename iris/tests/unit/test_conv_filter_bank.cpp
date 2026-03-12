#include <iris/nodes/conv_filter_bank.hpp>

#include <cmath>

#include <gtest/gtest.h>

using namespace iris;

// --- Helpers ---

static NormalizedIris make_input(int rows, int cols, double fill,
                                 uint8_t mask_fill = 1) {
    NormalizedIris ni;
    ni.normalized_image = cv::Mat(rows, cols, CV_64FC1, cv::Scalar(fill));
    ni.normalized_mask = cv::Mat(rows, cols, CV_8UC1, cv::Scalar(mask_fill));
    return ni;
}

static NormalizedIris make_gradient_input(int rows, int cols) {
    NormalizedIris ni;
    ni.normalized_image = cv::Mat(rows, cols, CV_64FC1);
    for (int r = 0; r < rows; ++r) {
        auto* row = ni.normalized_image.ptr<double>(r);
        for (int c = 0; c < cols; ++c) {
            row[c] = std::sin(static_cast<double>(c) * 0.1) *
                     std::cos(static_cast<double>(r) * 0.2);
        }
    }
    ni.normalized_mask = cv::Mat(rows, cols, CV_8UC1, cv::Scalar(1));
    return ni;
}

// --- Tests ---

TEST(ConvFilterBank, DefaultConstruction) {
    ConvFilterBank bank;
    EXPECT_EQ(bank.num_filters(), 2u);
    EXPECT_EQ(bank.params().iris_code_version, "v0.1");
    EXPECT_TRUE(bank.params().mask_is_duplicated);
}

TEST(ConvFilterBank, OutputShape) {
    // Use a single small filter for speed
    GaborFilter::Params fp;
    fp.kernel_size_phi = 9;
    fp.kernel_size_rho = 9;
    fp.to_fixpoints = false;

    RegularProbeSchema::Params sp;
    sp.n_rows = 8;
    sp.n_cols = 64;

    ConvFilterBank bank(
        ConvFilterBank::Params{},
        {GaborFilter{fp}},
        {RegularProbeSchema{sp}});

    auto input = make_gradient_input(64, 512);
    auto result = bank.run(input);
    ASSERT_TRUE(result.has_value());

    const auto& resp = result.value();
    ASSERT_EQ(resp.responses.size(), 1u);
    EXPECT_EQ(resp.responses[0].data.rows, 8);
    EXPECT_EQ(resp.responses[0].data.cols, 64);
    EXPECT_EQ(resp.responses[0].data.type(), CV_64FC2);
    EXPECT_EQ(resp.responses[0].mask.rows, 8);
    EXPECT_EQ(resp.responses[0].mask.cols, 64);
    EXPECT_EQ(resp.responses[0].mask.type(), CV_64FC2);
}

TEST(ConvFilterBank, UniformImageRealPartNearZero) {
    // DC correction makes the real kernel zero-mean per row, so a uniform
    // image produces near-zero real response at interior probes.
    // The imaginary part is also DC-adjusted (ratio * envelope subtracted),
    // so it won't be zero for uniform input — only real is tested here.
    GaborFilter::Params fp;
    fp.kernel_size_phi = 9;
    fp.kernel_size_rho = 9;
    fp.dc_correction = true;
    fp.to_fixpoints = false;

    RegularProbeSchema::Params sp;
    sp.n_rows = 8;
    sp.n_cols = 32;
    sp.boundary_rho_low = 0.1;
    sp.boundary_rho_high = 0.1;

    ConvFilterBank bank(
        ConvFilterBank::Params{},
        {GaborFilter{fp}},
        {RegularProbeSchema{sp}});

    auto input = make_input(64, 256, 0.5);
    auto result = bank.run(input);
    ASSERT_TRUE(result.has_value());

    const auto& data = result.value().responses[0].data;
    for (int r = 0; r < data.rows; ++r) {
        const auto* row = data.ptr<cv::Vec2d>(r);
        for (int c = 0; c < data.cols; ++c) {
            EXPECT_NEAR(row[c][0], 0.0, 1e-6)
                << "real at (" << r << "," << c << ")";
        }
    }
}

TEST(ConvFilterBank, GradientImageNonZero) {
    GaborFilter::Params fp;
    fp.kernel_size_phi = 9;
    fp.kernel_size_rho = 9;
    fp.to_fixpoints = false;

    RegularProbeSchema::Params sp;
    sp.n_rows = 4;
    sp.n_cols = 32;

    ConvFilterBank bank(
        ConvFilterBank::Params{},
        {GaborFilter{fp}},
        {RegularProbeSchema{sp}});

    auto input = make_gradient_input(64, 256);
    auto result = bank.run(input);
    ASSERT_TRUE(result.has_value());

    // At least some responses should be non-zero
    const auto& data = result.value().responses[0].data;
    double max_abs = 0.0;
    for (int r = 0; r < data.rows; ++r) {
        const auto* row = data.ptr<cv::Vec2d>(r);
        for (int c = 0; c < data.cols; ++c) {
            max_abs = std::max(max_abs,
                               std::max(std::abs(row[c][0]), std::abs(row[c][1])));
        }
    }
    EXPECT_GT(max_abs, 1e-10);
}

TEST(ConvFilterBank, MaskAllValid) {
    GaborFilter::Params fp;
    fp.kernel_size_phi = 9;
    fp.kernel_size_rho = 9;
    fp.to_fixpoints = false;

    RegularProbeSchema::Params sp;
    sp.n_rows = 4;
    sp.n_cols = 32;

    ConvFilterBank bank(
        ConvFilterBank::Params{},
        {GaborFilter{fp}},
        {RegularProbeSchema{sp}});

    auto input = make_gradient_input(64, 256);
    auto result = bank.run(input);
    ASSERT_TRUE(result.has_value());

    // With full mask, all non-zero responses should have mask = 1.0
    const auto& data = result.value().responses[0].data;
    const auto& mask = result.value().responses[0].mask;
    for (int r = 0; r < mask.rows; ++r) {
        const auto* dr = data.ptr<cv::Vec2d>(r);
        const auto* mr = mask.ptr<cv::Vec2d>(r);
        for (int c = 0; c < mask.cols; ++c) {
            if (std::abs(dr[c][0]) > 1e-15 || std::abs(dr[c][1]) > 1e-15) {
                EXPECT_DOUBLE_EQ(mr[c][0], 1.0)
                    << "mask real at (" << r << "," << c << ")";
                EXPECT_DOUBLE_EQ(mr[c][1], 1.0)
                    << "mask imag at (" << r << "," << c << ")";
            }
        }
    }
}

TEST(ConvFilterBank, MaskPartiallyInvalid) {
    GaborFilter::Params fp;
    fp.kernel_size_phi = 9;
    fp.kernel_size_rho = 9;
    fp.to_fixpoints = false;

    RegularProbeSchema::Params sp;
    sp.n_rows = 4;
    sp.n_cols = 32;

    ConvFilterBank bank(
        ConvFilterBank::Params{},
        {GaborFilter{fp}},
        {RegularProbeSchema{sp}});

    auto input = make_gradient_input(64, 256);
    // Zero out top half of mask
    input.normalized_mask(cv::Rect(0, 0, 256, 32)).setTo(0);

    auto result = bank.run(input);
    ASSERT_TRUE(result.has_value());

    // Probes in the top region should have mask < 1.0
    const auto& mask = result.value().responses[0].mask;
    const auto& data = result.value().responses[0].data;
    bool found_partial = false;
    for (int r = 0; r < mask.rows; ++r) {
        const auto* mr = mask.ptr<cv::Vec2d>(r);
        const auto* dr = data.ptr<cv::Vec2d>(r);
        for (int c = 0; c < mask.cols; ++c) {
            if ((std::abs(dr[c][0]) > 1e-15 || std::abs(dr[c][1]) > 1e-15) &&
                mr[c][0] < 1.0 - 1e-10) {
                found_partial = true;
            }
        }
    }
    EXPECT_TRUE(found_partial) << "Expected some partial mask values";
}

TEST(ConvFilterBank, MaskIsDuplicated) {
    GaborFilter::Params fp;
    fp.kernel_size_phi = 9;
    fp.kernel_size_rho = 9;
    fp.to_fixpoints = false;

    RegularProbeSchema::Params sp;
    sp.n_rows = 4;
    sp.n_cols = 32;

    ConvFilterBank::Params bp;
    bp.mask_is_duplicated = true;

    ConvFilterBank bank(bp, {GaborFilter{fp}}, {RegularProbeSchema{sp}});

    auto input = make_gradient_input(64, 256);
    input.normalized_mask(cv::Rect(0, 0, 256, 32)).setTo(0);

    auto result = bank.run(input);
    ASSERT_TRUE(result.has_value());

    // When duplicated, real and imaginary mask should be equal
    const auto& mask = result.value().responses[0].mask;
    for (int r = 0; r < mask.rows; ++r) {
        const auto* mr = mask.ptr<cv::Vec2d>(r);
        for (int c = 0; c < mask.cols; ++c) {
            EXPECT_DOUBLE_EQ(mr[c][0], mr[c][1])
                << "mask channels differ at (" << r << "," << c << ")";
        }
    }
}

TEST(ConvFilterBank, MaskNotDuplicated) {
    GaborFilter::Params fp;
    fp.kernel_size_phi = 9;
    fp.kernel_size_rho = 9;
    fp.to_fixpoints = false;

    RegularProbeSchema::Params sp;
    sp.n_rows = 4;
    sp.n_cols = 32;

    ConvFilterBank::Params bp;
    bp.mask_is_duplicated = false;

    ConvFilterBank bank(bp, {GaborFilter{fp}}, {RegularProbeSchema{sp}});

    auto input = make_gradient_input(64, 256);
    input.normalized_mask(cv::Rect(0, 0, 256, 32)).setTo(0);

    auto result = bank.run(input);
    ASSERT_TRUE(result.has_value());

    // Mask values should still be in [0, 1]
    const auto& mask = result.value().responses[0].mask;
    for (int r = 0; r < mask.rows; ++r) {
        const auto* mr = mask.ptr<cv::Vec2d>(r);
        for (int c = 0; c < mask.cols; ++c) {
            EXPECT_GE(mr[c][0], 0.0);
            EXPECT_LE(mr[c][0], 1.0 + 1e-10);
            EXPECT_GE(mr[c][1], 0.0);
            EXPECT_LE(mr[c][1], 1.0 + 1e-10);
        }
    }
}

TEST(ConvFilterBank, MultipleFilters) {
    GaborFilter::Params fp1;
    fp1.kernel_size_phi = 9;
    fp1.kernel_size_rho = 9;
    fp1.to_fixpoints = false;

    GaborFilter::Params fp2;
    fp2.kernel_size_phi = 7;
    fp2.kernel_size_rho = 7;
    fp2.sigma_phi = 2.0;
    fp2.lambda_phi = 8.0;
    fp2.to_fixpoints = false;

    RegularProbeSchema::Params sp;
    sp.n_rows = 4;
    sp.n_cols = 32;

    ConvFilterBank bank(
        ConvFilterBank::Params{},
        {GaborFilter{fp1}, GaborFilter{fp2}},
        {RegularProbeSchema{sp}, RegularProbeSchema{sp}});

    auto input = make_gradient_input(64, 256);
    auto result = bank.run(input);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().responses.size(), 2u);
}

TEST(ConvFilterBank, IrisCodeVersion) {
    ConvFilterBank::Params bp;
    bp.iris_code_version = "v3.0";

    GaborFilter::Params fp;
    fp.kernel_size_phi = 9;
    fp.kernel_size_rho = 9;
    fp.to_fixpoints = false;

    ConvFilterBank bank(bp, {GaborFilter{fp}}, {RegularProbeSchema{}});

    auto input = make_gradient_input(64, 256);
    auto result = bank.run(input);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().iris_code_version, "v3.0");
}

TEST(ConvFilterBank, EmptyImageFails) {
    ConvFilterBank bank;
    NormalizedIris empty;
    auto result = bank.run(empty);
    EXPECT_FALSE(result.has_value());
}

TEST(ConvFilterBank, ReproducibleOutput) {
    GaborFilter::Params fp;
    fp.kernel_size_phi = 9;
    fp.kernel_size_rho = 9;
    fp.to_fixpoints = false;

    RegularProbeSchema::Params sp;
    sp.n_rows = 4;
    sp.n_cols = 32;

    ConvFilterBank bank(
        ConvFilterBank::Params{},
        {GaborFilter{fp}},
        {RegularProbeSchema{sp}});

    auto input = make_gradient_input(64, 256);

    auto r1 = bank.run(input);
    auto r2 = bank.run(input);
    ASSERT_TRUE(r1.has_value() && r2.has_value());

    // Exact reproducibility
    cv::Mat diff;
    cv::absdiff(r1.value().responses[0].data, r2.value().responses[0].data, diff);
    EXPECT_EQ(cv::countNonZero(diff.reshape(1)), 0);
}
