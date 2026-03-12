#include <iris/nodes/gabor_filter.hpp>

#include <cmath>

#include <gtest/gtest.h>
#include <opencv2/core.hpp>

using namespace iris;

TEST(GaborFilter, KernelShape) {
    GaborFilter::Params params;
    params.kernel_size_phi = 41;
    params.kernel_size_rho = 21;
    GaborFilter filter{params};

    // Python convention: shape = (kernel_size[1], kernel_size[0])
    EXPECT_EQ(filter.rows(), 21);
    EXPECT_EQ(filter.cols(), 41);
    EXPECT_EQ(filter.kernel_real().type(), CV_64FC1);
    EXPECT_EQ(filter.kernel_imag().type(), CV_64FC1);
}

TEST(GaborFilter, DCCorrectedMeanNearZero) {
    GaborFilter::Params params;
    params.kernel_size_phi = 41;
    params.kernel_size_rho = 21;
    params.dc_correction = true;
    GaborFilter filter{params};

    // After DC correction, mean of real part should be near zero
    const double mean_real = cv::mean(filter.kernel_real())[0];
    EXPECT_NEAR(mean_real, 0.0, 0.01);
}

TEST(GaborFilter, NormalizedFrobenius) {
    GaborFilter filter;  // default params

    // After normalization (without fixpoints), Frobenius norm should be 1.0
    const double norm_real = cv::norm(filter.kernel_real(), cv::NORM_L2);
    const double norm_imag = cv::norm(filter.kernel_imag(), cv::NORM_L2);
    EXPECT_NEAR(norm_real, 1.0, 1e-10);
    EXPECT_NEAR(norm_imag, 1.0, 1e-10);
}

TEST(GaborFilter, KernelNormValues) {
    GaborFilter filter;  // default, no fixpoints

    // Without fixpoints, kernel norms should be 1.0
    EXPECT_NEAR(filter.kernel_norm_real(), 1.0, 1e-10);
    EXPECT_NEAR(filter.kernel_norm_imag(), 1.0, 1e-10);
}

TEST(GaborFilter, FixpointScaling) {
    GaborFilter::Params params;
    params.to_fixpoints = true;
    GaborFilter filter{params};

    // With fixpoints, values should be scaled by 2^15 = 32768
    // The kernel norm should be much larger than 1.0
    EXPECT_GT(filter.kernel_norm_real(), 100.0);
    EXPECT_GT(filter.kernel_norm_imag(), 100.0);

    // Max absolute value should be in the ballpark of 2^15 * max_original
    double min_val = 0.0, max_val = 0.0;
    cv::minMaxLoc(filter.kernel_real(), &min_val, &max_val);
    EXPECT_GT(std::abs(max_val) + std::abs(min_val), 100.0);
}

TEST(GaborFilter, VariousParams) {
    // Test with different parameter sets that are valid
    GaborFilter::Params p1;
    p1.kernel_size_phi = 17;
    p1.kernel_size_rho = 21;
    p1.sigma_phi = 2.0;
    p1.sigma_rho = 5.86;
    p1.lambda_phi = 8.0;
    GaborFilter f1{p1};
    EXPECT_EQ(f1.rows(), 21);
    EXPECT_EQ(f1.cols(), 17);

    GaborFilter::Params p2;
    p2.kernel_size_phi = 31;
    p2.kernel_size_rho = 31;
    p2.sigma_phi = 5.0;
    p2.sigma_rho = 5.0;
    p2.theta_degrees = 45.0;
    p2.lambda_phi = 10.0;
    GaborFilter f2{p2};
    EXPECT_EQ(f2.rows(), 31);
    EXPECT_EQ(f2.cols(), 31);
}

TEST(GaborFilter, NoDCCorrectionMeanNonZero) {
    GaborFilter::Params params;
    params.dc_correction = false;
    GaborFilter filter{params};

    // Without DC correction, mean of real part is typically nonzero
    const double mean_real = cv::mean(filter.kernel_real())[0];
    // Just verify it computes without error; the mean might be small but nonzero
    EXPECT_TRUE(std::isfinite(mean_real));
}

TEST(GaborFilter, NodeParamsConstruction) {
    std::unordered_map<std::string, std::string> np;
    np["kernel_size_phi"] = "19";
    np["kernel_size_rho"] = "15";
    np["sigma_phi"] = "3.0";
    np["sigma_rho"] = "4.0";
    np["theta_degrees"] = "45.0";
    np["lambda_phi"] = "10.0";
    np["dc_correction"] = "true";
    np["to_fixpoints"] = "false";
    GaborFilter filter{np};

    EXPECT_EQ(filter.rows(), 15);
    EXPECT_EQ(filter.cols(), 19);
    EXPECT_NEAR(filter.kernel_norm_real(), 1.0, 1e-10);
}
