#pragma once

#include <string_view>
#include <unordered_map>

#include <opencv2/core.hpp>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>

namespace iris {

/// 2D Log-Gabor filter for iris texture analysis (frequency domain).
///
/// Computes filter kernel in the frequency domain using a log-Gabor radial
/// envelope and a Gaussian angular component, then applies iFFT to obtain
/// the spatial-domain kernel.
///
/// Reference: https://en.wikipedia.org/wiki/Log_Gabor_filter
class LogGaborFilter : public Algorithm<LogGaborFilter> {
public:
    static constexpr std::string_view kName = "LogGaborFilter";

    struct Params {
        int kernel_size_phi = 41;      // kernel width (phi direction)
        int kernel_size_rho = 21;      // kernel height (rho direction)
        double sigma_phi = 0.7;        // angular bandwidth (0 < σ ≤ π)
        double sigma_rho = 0.5;        // radial bandwidth (0.1 < σ ≤ 1)
        double theta_degrees = 90.0;   // orientation in degrees
        double lambda_rho = 8.0;       // wavelength in rho
        bool to_fixpoints = false;
    };

    LogGaborFilter() { compute_kernel(); }
    explicit LogGaborFilter(const Params& params) : params_(params) {
        compute_kernel();
    }
    explicit LogGaborFilter(
        const std::unordered_map<std::string, std::string>& node_params);

    const cv::Mat& kernel_real() const noexcept { return kernel_real_; }
    const cv::Mat& kernel_imag() const noexcept { return kernel_imag_; }
    double kernel_norm_real() const noexcept { return kernel_norm_real_; }
    double kernel_norm_imag() const noexcept { return kernel_norm_imag_; }
    int rows() const noexcept { return kernel_real_.rows; }
    int cols() const noexcept { return kernel_real_.cols; }
    const Params& params() const noexcept { return params_; }

private:
    Params params_;
    cv::Mat kernel_real_;   // CV_64FC1, shape (kernel_size_rho, kernel_size_phi)
    cv::Mat kernel_imag_;   // CV_64FC1, same shape
    double kernel_norm_real_ = 0.0;
    double kernel_norm_imag_ = 0.0;

    void compute_kernel();
};

}  // namespace iris
