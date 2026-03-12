#include <iris/nodes/gabor_filter.hpp>

#include <cmath>
#include <numbers>
#include <string>

#include <iris/core/node_registry.hpp>
#include <iris/utils/filter_utils.hpp>

namespace iris {

GaborFilter::GaborFilter(
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

    it = node_params.find("lambda_phi");
    if (it != node_params.end()) params_.lambda_phi = std::stod(it->second);

    it = node_params.find("dc_correction");
    if (it != node_params.end()) params_.dc_correction = (it->second == "true" || it->second == "1");

    it = node_params.find("to_fixpoints");
    if (it != node_params.end()) params_.to_fixpoints = (it->second == "true" || it->second == "1");

    compute_kernel();
}

void GaborFilter::compute_kernel() {
    const int phi_size = params_.kernel_size_phi;
    const int rho_size = params_.kernel_size_rho;

    // Create centered coordinate meshgrid
    auto [x, y] = filter_utils::make_meshgrid(phi_size, rho_size);

    // Rotate coordinates by theta
    auto [rotx, roty] = filter_utils::rotate_coords(x, y, params_.theta_degrees);

    // Compute envelope exponent: -(rotx^2/sigma_phi^2 + roty^2/sigma_rho^2)/2
    // and carrier: 2*pi/lambda * rotx
    const double inv_sp2 = 1.0 / (params_.sigma_phi * params_.sigma_phi);
    const double inv_sr2 = 1.0 / (params_.sigma_rho * params_.sigma_rho);
    const double carrier_freq = 2.0 * std::numbers::pi / params_.lambda_phi;
    const double norm_factor = 1.0 / (2.0 * std::numbers::pi * params_.sigma_phi * params_.sigma_rho);

    // envelope stores the exponent (negative values)
    cv::Mat envelope(rho_size, phi_size, CV_64FC1);
    kernel_real_ = cv::Mat(rho_size, phi_size, CV_64FC1);
    kernel_imag_ = cv::Mat(rho_size, phi_size, CV_64FC1);

    for (int r = 0; r < rho_size; ++r) {
        const auto* rx = rotx.ptr<double>(r);
        const auto* ry = roty.ptr<double>(r);
        auto* env = envelope.ptr<double>(r);
        auto* kr = kernel_real_.ptr<double>(r);
        auto* ki = kernel_imag_.ptr<double>(r);

        for (int c = 0; c < phi_size; ++c) {
            const double env_val = -(rx[c] * rx[c] * inv_sp2
                                     + ry[c] * ry[c] * inv_sr2) / 2.0;
            env[c] = env_val;

            const double carrier_arg = carrier_freq * rx[c];
            const double exp_env = std::exp(env_val);

            // kernel = exp(envelope + j*carrier) / (2*pi*sigma_phi*sigma_rho)
            kr[c] = exp_env * std::cos(carrier_arg) * norm_factor;
            ki[c] = exp_env * std::sin(carrier_arg) * norm_factor;
        }
    }

    // DC correction: subtract Gaussian-weighted mean per row (real part only).
    // In Python, subtracting a real correction from a complex kernel only
    // affects the real component. The imaginary part must not be modified.
    if (params_.dc_correction) {
        for (int r = 0; r < rho_size; ++r) {
            const auto* env = envelope.ptr<double>(r);
            auto* kr = kernel_real_.ptr<double>(r);

            // Mean of real part along phi (columns)
            double sum_real = 0.0;
            double sum_env = 0.0;
            for (int c = 0; c < phi_size; ++c) {
                sum_real += kr[c];
                sum_env += env[c];
            }

            if (std::abs(sum_env) > 1e-30) {
                const double ratio = sum_real / sum_env;
                for (int c = 0; c < phi_size; ++c) {
                    kr[c] -= ratio * env[c];
                }
            }
        }
    }

    // Normalize Frobenius norms to 1.0
    filter_utils::normalize_frobenius(kernel_real_, kernel_imag_);

    // Optional fixed-point conversion
    if (params_.to_fixpoints) {
        filter_utils::apply_fixpoints(kernel_real_, kernel_imag_);
    }

    // Compute final kernel norms (after normalization and optional fixpoint)
    kernel_norm_real_ = cv::norm(kernel_real_, cv::NORM_L2);
    kernel_norm_imag_ = cv::norm(kernel_imag_, cv::NORM_L2);
}

IRIS_REGISTER_NODE("iris.GaborFilter", GaborFilter);

}  // namespace iris
