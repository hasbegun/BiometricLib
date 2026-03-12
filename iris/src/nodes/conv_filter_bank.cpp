#include <iris/nodes/conv_filter_bank.hpp>

#include <algorithm>
#include <cmath>

#include <iris/core/node_registry.hpp>

namespace iris {

// ---------------------------------------------------------------------------
// Default constructor: two Gabor filters matching pipeline.yaml defaults
// ---------------------------------------------------------------------------
ConvFilterBank::ConvFilterBank() {
    // Filter 1: wide phi, narrow lambda
    GaborFilter::Params f1;
    f1.kernel_size_phi = 41;
    f1.kernel_size_rho = 21;
    f1.sigma_phi = 7.0;
    f1.sigma_rho = 6.13;
    f1.theta_degrees = 90.0;
    f1.lambda_phi = 28.0;
    f1.dc_correction = true;
    f1.to_fixpoints = true;
    filters_.emplace_back(f1);

    // Filter 2: narrow phi, short lambda
    GaborFilter::Params f2;
    f2.kernel_size_phi = 17;
    f2.kernel_size_rho = 21;
    f2.sigma_phi = 2.0;
    f2.sigma_rho = 5.86;
    f2.theta_degrees = 90.0;
    f2.lambda_phi = 8.0;
    f2.dc_correction = true;
    f2.to_fixpoints = true;
    filters_.emplace_back(f2);

    schemas_.emplace_back(RegularProbeSchema{});
    schemas_.emplace_back(RegularProbeSchema{});
}

ConvFilterBank::ConvFilterBank(Params params,
                               std::vector<GaborFilter> filters,
                               std::vector<RegularProbeSchema> schemas)
    : params_(std::move(params)),
      filters_(std::move(filters)),
      schemas_(std::move(schemas)) {}

ConvFilterBank::ConvFilterBank(
    const std::unordered_map<std::string, std::string>& node_params) {
    // Use default filters/schemas first
    *this = ConvFilterBank{};

    // Apply parsed params (check both YAML form and snake_case form)
    if (auto v = node_params.find("iris_code_version"); v != node_params.end()) {
        params_.iris_code_version = v->second;
    }
    // YAML uses "maskisduplicated", also accept "mask_is_duplicated"
    if (auto m = node_params.find("maskisduplicated"); m != node_params.end()) {
        params_.mask_is_duplicated = (m->second == "true" || m->second == "1");
    } else if (auto m2 = node_params.find("mask_is_duplicated");
               m2 != node_params.end()) {
        params_.mask_is_duplicated = (m2->second == "true" || m2->second == "1");
    }
}

// ---------------------------------------------------------------------------
// polar_pad: zero-pad vertically (implicit — no extra rows), circular wrap
//            horizontally by pad_cols on each side.
// ---------------------------------------------------------------------------
cv::Mat ConvFilterBank::polar_pad(const cv::Mat& img, int pad_cols) {
    const int rows = img.rows;
    const int cols = img.cols;
    const int padded_cols = cols + 2 * pad_cols;

    cv::Mat padded = cv::Mat::zeros(rows, padded_cols, img.type());

    // Center: copy original image
    img.copyTo(padded(cv::Rect(pad_cols, 0, cols, rows)));

    // Left wrap: rightmost pad_cols columns → left padding
    img(cv::Rect(cols - pad_cols, 0, pad_cols, rows))
        .copyTo(padded(cv::Rect(0, 0, pad_cols, rows)));

    // Right wrap: leftmost pad_cols columns → right padding
    img(cv::Rect(0, 0, pad_cols, rows))
        .copyTo(padded(cv::Rect(pad_cols + cols, 0, pad_cols, rows)));

    return padded;
}

// ---------------------------------------------------------------------------
// convolve: apply one filter at all probe positions
// ---------------------------------------------------------------------------
IrisFilterResponse::Response ConvFilterBank::convolve(
    const GaborFilter& filter,
    const RegularProbeSchema& schema,
    const NormalizedIris& input) const {
    const int i_rows = input.normalized_image.rows;
    const int i_cols = input.normalized_image.cols;
    const int k_rows = filter.rows();
    const int k_cols = filter.cols();
    const int p_rows = k_rows / 2;
    const int p_cols = k_cols / 2;
    const int n_rows = schema.params().n_rows;
    const int n_cols = schema.params().n_cols;

    // Output: CV_64FC2 (real + imaginary)
    cv::Mat iris_data(n_rows, n_cols, CV_64FC2, cv::Scalar(0.0, 0.0));
    cv::Mat mask_data(n_rows, n_cols, CV_64FC2, cv::Scalar(0.0, 0.0));

    // Pad only horizontally (circular wrap for phi periodicity)
    const cv::Mat padded_iris = polar_pad(input.normalized_image, p_cols);

    // Convert mask to double for the same padding operation.
    // normalized_mask is CV_8UC1 with values 0 or 1 (not 0/255).
    cv::Mat mask_double;
    input.normalized_mask.convertTo(mask_double, CV_64FC1);
    const cv::Mat padded_mask = polar_pad(mask_double, p_cols);

    const auto& rhos = schema.rhos();
    const auto& phis = schema.phis();

    for (int i = 0; i < n_rows; ++i) {
        auto* data_row = iris_data.ptr<cv::Vec2d>(i);
        auto* mask_row = mask_data.ptr<cv::Vec2d>(i);

        for (int j = 0; j < n_cols; ++j) {
            const int pos = i * n_cols + j;

            // Convert normalized probe coords to pixel positions
            int r_probe = static_cast<int>(
                std::min(std::round(rhos[static_cast<size_t>(pos)] *
                                    static_cast<double>(i_rows)),
                         static_cast<double>(i_rows - 1)));
            int c_probe = static_cast<int>(
                std::min(std::round(phis[static_cast<size_t>(pos)] *
                                    static_cast<double>(i_cols)),
                         static_cast<double>(i_cols - 1)));

            // Vertical clipping (no vertical padding)
            const int rtop = std::max(0, r_probe - p_rows);
            // Python uses i_rows - 1 to exclude the bottom row
            const int rbot = std::min(r_probe + p_rows + 1, i_rows - 1);
            const int patch_rows = rbot - rtop;
            if (patch_rows <= 0) continue;

            // Extract patches from padded images (horizontal padding handles
            // circular wrap)
            cv::Mat iris_patch =
                padded_iris(cv::Rect(c_probe, rtop, k_cols, patch_rows));
            cv::Mat mask_patch =
                padded_mask(cv::Rect(c_probe, rtop, k_cols, patch_rows));

            // Corresponding filter region (clip vertically to match patch)
            const int ktop = p_rows - (patch_rows / 2);
            cv::Mat filter_real =
                filter.kernel_real()(cv::Rect(0, ktop, k_cols, patch_rows));
            cv::Mat filter_imag =
                filter.kernel_imag()(cv::Rect(0, ktop, k_cols, patch_rows));

            // Iris response: sum(image * filter) / patch_rows / k_cols
            const double norm_factor =
                1.0 / static_cast<double>(patch_rows) /
                static_cast<double>(k_cols);
            const double resp_real = iris_patch.dot(filter_real) * norm_factor;
            const double resp_imag = iris_patch.dot(filter_imag) * norm_factor;

            data_row[j] = cv::Vec2d(resp_real, resp_imag);

            // Mask response: fraction of filter energy at valid pixels
            if (resp_real == 0.0 && resp_imag == 0.0) {
                mask_row[j] = cv::Vec2d(0.0, 0.0);
            } else {
                // Check if all mask pixels are valid
                bool all_valid = true;
                for (int mr = 0; mr < mask_patch.rows && all_valid; ++mr) {
                    const auto* mp = mask_patch.ptr<double>(mr);
                    for (int mc = 0; mc < mask_patch.cols && all_valid; ++mc) {
                        if (mp[mc] < 0.5) all_valid = false;
                    }
                }

                if (all_valid) {
                    mask_row[j] = cv::Vec2d(1.0, 1.0);
                } else if (params_.mask_is_duplicated) {
                    // Use imaginary filter part for both channels
                    double valid_sum = 0.0;
                    double total_sum = 0.0;
                    for (int mr = 0; mr < patch_rows; ++mr) {
                        const auto* mp = mask_patch.ptr<double>(mr);
                        const auto* fp = filter_imag.ptr<double>(mr);
                        for (int mc = 0; mc < k_cols; ++mc) {
                            const double af = std::abs(fp[mc]);
                            total_sum += af;
                            if (mp[mc] >= 0.5) {
                                valid_sum += af;
                            }
                        }
                    }
                    const double val =
                        total_sum > 0.0 ? valid_sum / total_sum : 0.0;
                    mask_row[j] = cv::Vec2d(val, val);
                } else {
                    // Separate real/imag mask values
                    double valid_real = 0.0, total_real = 0.0;
                    double valid_imag = 0.0, total_imag = 0.0;
                    for (int mr = 0; mr < patch_rows; ++mr) {
                        const auto* mp = mask_patch.ptr<double>(mr);
                        const auto* fr = filter_real.ptr<double>(mr);
                        const auto* fi = filter_imag.ptr<double>(mr);
                        for (int mc = 0; mc < k_cols; ++mc) {
                            const double ar = std::abs(fr[mc]);
                            const double ai = std::abs(fi[mc]);
                            total_real += ar;
                            total_imag += ai;
                            if (mp[mc] >= 0.5) {
                                valid_real += ar;
                                valid_imag += ai;
                            }
                        }
                    }
                    const double vr =
                        total_real > 0.0 ? valid_real / total_real : 0.0;
                    const double vi =
                        total_imag > 0.0 ? valid_imag / total_imag : 0.0;
                    mask_row[j] = cv::Vec2d(vr, vi);
                }
            }
        }
    }

    // Final normalization by kernel norms
    const double norm_real = filter.kernel_norm_real();
    const double norm_imag = filter.kernel_norm_imag();

    for (int r = 0; r < n_rows; ++r) {
        auto* row = iris_data.ptr<cv::Vec2d>(r);
        for (int c = 0; c < n_cols; ++c) {
            if (norm_real > 0.0) row[c][0] /= norm_real;
            if (norm_imag > 0.0) row[c][1] /= norm_imag;
        }
    }

    return {iris_data, mask_data};
}

// ---------------------------------------------------------------------------
// run
// ---------------------------------------------------------------------------
Result<IrisFilterResponse> ConvFilterBank::run(const NormalizedIris& input) const {
    if (filters_.size() != schemas_.size()) {
        return make_error(ErrorCode::ConfigInvalid,
                          "Number of filters must match number of probe schemas");
    }
    if (filters_.empty()) {
        return make_error(ErrorCode::ConfigInvalid,
                          "At least one filter-schema pair required");
    }
    if (input.normalized_image.empty()) {
        return make_error(ErrorCode::EncodingFailed,
                          "Normalized image is empty");
    }

    IrisFilterResponse output;
    output.iris_code_version = params_.iris_code_version;

    for (size_t idx = 0; idx < filters_.size(); ++idx) {
        output.responses.push_back(
            convolve(filters_[idx], schemas_[idx], input));
    }

    return output;
}

IRIS_REGISTER_NODE("iris.ConvFilterBank", ConvFilterBank);

}  // namespace iris
