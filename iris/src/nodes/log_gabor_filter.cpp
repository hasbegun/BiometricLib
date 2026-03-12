#include <iris/nodes/log_gabor_filter.hpp>

#include <cmath>
#include <numbers>
#include <string>

#include <opencv2/core.hpp>

#include <iris/core/node_registry.hpp>
#include <iris/utils/filter_utils.hpp>

namespace iris {

LogGaborFilter::LogGaborFilter(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("kernel_size_phi");
    if (it != node_params.end()) params_.kernel_size_phi = std::stoi(it->second);

    it = node_params.find("kernel_size_rho");
    if (it != node_params.end()) params_.kernel_size_rho = std::stoi(it->second);

    it = node_params.find("sigma_phi");
    if (it != node_params.end()) params_.sigma_phi = std::stod(it->second);

    it = node_params.find("sigma_rho");
    if (it != node_params.end()) params_.sigma_rho = std::stod(it->second);

    it = node_params.find("theta_degrees");
    if (it != node_params.end()) params_.theta_degrees = std::stod(it->second);

    it = node_params.find("lambda_rho");
    if (it != node_params.end()) params_.lambda_rho = std::stod(it->second);

    it = node_params.find("to_fixpoints");
    if (it != node_params.end()) params_.to_fixpoints = (it->second == "true" || it->second == "1");

    compute_kernel();
}

void LogGaborFilter::compute_kernel() {
    const int phi_size = params_.kernel_size_phi;
    const int rho_size = params_.kernel_size_rho;
    const int phi_half = phi_size / 2;
    const int rho_half = rho_size / 2;

    // Create centered coordinate meshgrid
    auto [x, y] = filter_utils::make_meshgrid(phi_size, rho_size);

    // Compute radius to center
    cv::Mat radius = filter_utils::compute_radius(x, y);

    // Set center to 1 to avoid log(0)
    radius.at<double>(rho_half, phi_half) = 1.0;

    // Get angular distance via rotated coordinates
    auto [rotx, roty] = filter_utils::rotate_coords(x, y, params_.theta_degrees);

    // dtheta = atan2(roty, rotx) — angular distance from filter orientation
    cv::Mat dtheta(rho_size, phi_size, CV_64FC1);
    for (int r = 0; r < rho_size; ++r) {
        const auto* rxr = rotx.ptr<double>(r);
        const auto* ryr = roty.ptr<double>(r);
        auto* dt = dtheta.ptr<double>(r);
        for (int c = 0; c < phi_size; ++c) {
            dt[c] = std::atan2(ryr[c], rxr[c]);
        }
    }

    // Build frequency-domain filter:
    //   envelope = exp(-0.5 * log2(radius * lambda_rho / kernel_size_rho)^2 / sigma_rho^2)
    //   orientation = exp(-0.5 * dtheta^2 / sigma_phi^2)
    //   filter = envelope * orientation
    const double inv_sigma_rho2 = 1.0 / (params_.sigma_rho * params_.sigma_rho);
    const double inv_sigma_phi2 = 1.0 / (params_.sigma_phi * params_.sigma_phi);
    const double scale_factor = params_.lambda_rho / static_cast<double>(rho_size);

    cv::Mat freq_filter(rho_size, phi_size, CV_64FC1);
    for (int r = 0; r < rho_size; ++r) {
        const auto* rad = radius.ptr<double>(r);
        const auto* dt = dtheta.ptr<double>(r);
        auto* ff = freq_filter.ptr<double>(r);
        for (int c = 0; c < phi_size; ++c) {
            const double log_term = std::log2(rad[c] * scale_factor);
            const double env = std::exp(-0.5 * log_term * log_term * inv_sigma_rho2);
            const double orient = std::exp(-0.5 * dt[c] * dt[c] * inv_sigma_phi2);
            ff[c] = env * orient;
        }
    }

    // Zero out the center (was set to 1 to avoid log(0))
    freq_filter.at<double>(rho_half, phi_half) = 0.0;

    // Inverse FFT: kernel = fftshift(ifft2(ifftshift(freq_filter)))
    // Step 1: ifftshift — swap quadrants (same as fftshift for even-sized)
    cv::Mat shifted = filter_utils::fftshift(freq_filter);

    // Step 2: ifft2 — inverse 2D DFT
    // cv::dft with DFT_INVERSE expects complex input → pad with zero imaginary
    cv::Mat planes[] = {shifted, cv::Mat::zeros(rho_size, phi_size, CV_64FC1)};
    cv::Mat complex_in;
    cv::merge(planes, 2, complex_in);

    cv::Mat complex_out;
    cv::dft(complex_in, complex_out, cv::DFT_INVERSE | cv::DFT_SCALE);

    // Step 3: Split real and imaginary parts
    cv::Mat out_planes[2];
    cv::split(complex_out, out_planes);

    // Step 4: fftshift the result
    kernel_real_ = filter_utils::fftshift(out_planes[0]);
    kernel_imag_ = filter_utils::fftshift(out_planes[1]);

    // Normalize Frobenius norms to 1.0
    filter_utils::normalize_frobenius(kernel_real_, kernel_imag_);

    // Optional fixed-point conversion
    if (params_.to_fixpoints) {
        filter_utils::apply_fixpoints(kernel_real_, kernel_imag_);
    }

    // Compute final kernel norms
    kernel_norm_real_ = cv::norm(kernel_real_, cv::NORM_L2);
    kernel_norm_imag_ = cv::norm(kernel_imag_, cv::NORM_L2);
}

IRIS_REGISTER_NODE("iris.LogGaborFilter", LogGaborFilter);

}  // namespace iris
