#pragma once

#include <string_view>
#include <unordered_map>

#include <opencv2/core.hpp>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>

namespace iris {

/// 2D Gabor filter for iris texture analysis.
/// Precomputes kernel at construction time.
class GaborFilter : public Algorithm<GaborFilter> {
public:
    static constexpr std::string_view kName = "GaborFilter";

    struct Params {
        int kernel_size_phi = 41;   // kernel width (phi direction)
        int kernel_size_rho = 21;   // kernel height (rho direction)
        double sigma_phi = 7.0;
        double sigma_rho = 6.13;
        double theta_degrees = 90.0;
        double lambda_phi = 28.0;
        bool dc_correction = true;
        bool to_fixpoints = false;
    };

    GaborFilter() { compute_kernel(); }
    explicit GaborFilter(const Params& params) : params_(params) {
        compute_kernel();
    }
    explicit GaborFilter(
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
