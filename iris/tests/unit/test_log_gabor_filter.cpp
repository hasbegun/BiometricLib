#include <cmath>
#include <unordered_map>

#include <iris/nodes/log_gabor_filter.hpp>

#include <gtest/gtest.h>

namespace iris::test {

TEST(LogGaborFilter, DefaultConstructionProducesNonZeroKernel) {
    LogGaborFilter filter;
    EXPECT_GT(cv::norm(filter.kernel_real(), cv::NORM_L2), 0.0);
    EXPECT_GT(cv::norm(filter.kernel_imag(), cv::NORM_L2), 0.0);
}

TEST(LogGaborFilter, KernelDimensionsMatchParams) {
    LogGaborFilter::Params params;
    params.kernel_size_phi = 31;
    params.kernel_size_rho = 15;
    LogGaborFilter filter(params);

    EXPECT_EQ(filter.rows(), 15);
    EXPECT_EQ(filter.cols(), 31);
    EXPECT_EQ(filter.kernel_real().rows, 15);
    EXPECT_EQ(filter.kernel_real().cols, 31);
    EXPECT_EQ(filter.kernel_imag().rows, 15);
    EXPECT_EQ(filter.kernel_imag().cols, 31);
}

TEST(LogGaborFilter, KernelIsNormalized) {
    LogGaborFilter filter;
    // After normalize_frobenius, each component should have Frobenius norm ≈ 1.0
    double norm_real = cv::norm(filter.kernel_real(), cv::NORM_L2);
    double norm_imag = cv::norm(filter.kernel_imag(), cv::NORM_L2);
    EXPECT_NEAR(norm_real, 1.0, 1e-10);
    EXPECT_NEAR(norm_imag, 1.0, 1e-10);
}

TEST(LogGaborFilter, FixpointsMode) {
    LogGaborFilter::Params params;
    params.to_fixpoints = true;
    LogGaborFilter filter(params);

    // In fixpoints mode, values should be integer-like (multiples of 1.0)
    bool all_integer_like = true;
    for (int r = 0; r < filter.rows(); ++r) {
        const auto* kr = filter.kernel_real().ptr<double>(r);
        const auto* ki = filter.kernel_imag().ptr<double>(r);
        for (int c = 0; c < filter.cols(); ++c) {
            if (std::abs(kr[c] - std::round(kr[c])) > 1e-10) all_integer_like = false;
            if (std::abs(ki[c] - std::round(ki[c])) > 1e-10) all_integer_like = false;
        }
    }
    EXPECT_TRUE(all_integer_like);
}

TEST(LogGaborFilter, DifferentThetaProducesDifferentKernels) {
    LogGaborFilter::Params p1;
    p1.theta_degrees = 0.0;
    LogGaborFilter f1(p1);

    LogGaborFilter::Params p2;
    p2.theta_degrees = 90.0;
    LogGaborFilter f2(p2);

    // Kernels should differ
    cv::Mat diff;
    cv::absdiff(f1.kernel_real(), f2.kernel_real(), diff);
    double max_diff = 0.0;
    cv::minMaxLoc(diff, nullptr, &max_diff);
    EXPECT_GT(max_diff, 1e-6);
}

TEST(LogGaborFilter, NoNaNOrInf) {
    LogGaborFilter filter;
    for (int r = 0; r < filter.rows(); ++r) {
        const auto* kr = filter.kernel_real().ptr<double>(r);
        const auto* ki = filter.kernel_imag().ptr<double>(r);
        for (int c = 0; c < filter.cols(); ++c) {
            EXPECT_FALSE(std::isnan(kr[c])) << "NaN at real(" << r << "," << c << ")";
            EXPECT_FALSE(std::isinf(kr[c])) << "Inf at real(" << r << "," << c << ")";
            EXPECT_FALSE(std::isnan(ki[c])) << "NaN at imag(" << r << "," << c << ")";
            EXPECT_FALSE(std::isinf(ki[c])) << "Inf at imag(" << r << "," << c << ")";
        }
    }
}

TEST(LogGaborFilter, ConstructFromNodeParams) {
    std::unordered_map<std::string, std::string> params = {
        {"kernel_size_phi", "31"},
        {"kernel_size_rho", "15"},
        {"sigma_phi", "0.5"},
        {"sigma_rho", "0.3"},
        {"theta_degrees", "45.0"},
        {"lambda_rho", "6.0"},
        {"to_fixpoints", "false"},
    };
    LogGaborFilter filter(params);

    EXPECT_EQ(filter.params().kernel_size_phi, 31);
    EXPECT_EQ(filter.params().kernel_size_rho, 15);
    EXPECT_DOUBLE_EQ(filter.params().sigma_phi, 0.5);
    EXPECT_DOUBLE_EQ(filter.params().sigma_rho, 0.3);
    EXPECT_DOUBLE_EQ(filter.params().theta_degrees, 45.0);
    EXPECT_DOUBLE_EQ(filter.params().lambda_rho, 6.0);
    EXPECT_FALSE(filter.params().to_fixpoints);
}

}  // namespace iris::test
