#include <iris/nodes/fragile_bit_refinement.hpp>

#include <cmath>

#include <iris/core/node_registry.hpp>

namespace iris {

FragileBitRefinement::FragileBitRefinement(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("fragile_type");
    if (it != node_params.end()) {
        params_.fragile_type = it->second;
    }
    // YAML uses "maskisduplicated", also accept "mask_is_duplicated"
    it = node_params.find("maskisduplicated");
    if (it != node_params.end()) {
        params_.mask_is_duplicated = (it->second == "true" || it->second == "1");
    } else {
        it = node_params.find("mask_is_duplicated");
        if (it != node_params.end()) {
            params_.mask_is_duplicated = (it->second == "true" || it->second == "1");
        }
    }
    // value_threshold parsing: "0.0001,0.275,0.0873"
    it = node_params.find("value_threshold");
    if (it != node_params.end()) {
        // Simple CSV parse
        const auto& s = it->second;
        size_t start = 0;
        for (size_t idx = 0; idx < 3; ++idx) {
            auto end = s.find(',', start);
            params_.value_threshold[idx] = std::stod(s.substr(start, end - start));
            start = (end == std::string::npos) ? s.size() : end + 1;
        }
    }
}

// ---------------------------------------------------------------------------
// Polar mode: threshold on radius and angle
// ---------------------------------------------------------------------------
cv::Mat FragileBitRefinement::refine_polar(const cv::Mat& data,
                                           const cv::Mat& mask) const {
    const int rows = data.rows;
    const int cols = data.cols;
    const double r_min = params_.value_threshold[0];
    const double r_max = params_.value_threshold[1];
    const double angle_thresh = params_.value_threshold[2];
    const double cos_thresh = std::abs(std::cos(angle_thresh));

    cv::Mat refined(rows, cols, CV_64FC2, cv::Scalar(0.0, 0.0));

    for (int r = 0; r < rows; ++r) {
        const auto* d = data.ptr<cv::Vec2d>(r);
        const auto* m = mask.ptr<cv::Vec2d>(r);
        auto* out = refined.ptr<cv::Vec2d>(r);

        for (int c = 0; c < cols; ++c) {
            const double re = d[c][0];
            const double im = d[c][1];

            // Radius = |z|
            const double radius = std::sqrt(re * re + im * im);

            // Radius check
            const bool r_ok = (radius >= r_min) && (radius <= r_max);

            if (!r_ok) {
                out[c] = cv::Vec2d(0.0, 0.0);
                continue;
            }

            // Angle = atan2(imag, real)
            const double phi = std::atan2(im, re);

            if (params_.mask_is_duplicated) {
                // Use cosine angle check, duplicate to both channels
                const bool cos_ok = std::abs(std::cos(phi)) <= cos_thresh;
                const double val = cos_ok ? m[c][1] : 0.0;
                out[c] = cv::Vec2d(val, val);
            } else {
                // Separate angle checks for real and imaginary
                const bool cos_ok = std::abs(std::cos(phi)) <= cos_thresh;
                const bool sin_ok = std::abs(std::sin(phi)) <= cos_thresh;
                const double val_real = sin_ok ? m[c][0] : 0.0;
                const double val_imag = cos_ok ? m[c][1] : 0.0;
                out[c] = cv::Vec2d(val_real, val_imag);
            }
        }
    }

    return refined;
}

// ---------------------------------------------------------------------------
// Cartesian mode: threshold on |real| and |imag| separately
// ---------------------------------------------------------------------------
cv::Mat FragileBitRefinement::refine_cartesian(const cv::Mat& data,
                                               const cv::Mat& mask) const {
    const int rows = data.rows;
    const int cols = data.cols;
    const double t_min = params_.value_threshold[0];
    const double t_max_real = params_.value_threshold[1];
    const double t_max_imag = params_.value_threshold[2];

    cv::Mat refined(rows, cols, CV_64FC2, cv::Scalar(0.0, 0.0));

    for (int r = 0; r < rows; ++r) {
        const auto* d = data.ptr<cv::Vec2d>(r);
        const auto* m = mask.ptr<cv::Vec2d>(r);
        auto* out = refined.ptr<cv::Vec2d>(r);

        for (int c = 0; c < cols; ++c) {
            const double abs_real = std::abs(d[c][0]);
            const double abs_imag = std::abs(d[c][1]);

            const bool imag_ok = (abs_imag >= t_min) && (abs_imag <= t_max_imag);

            if (params_.mask_is_duplicated) {
                const double val = imag_ok ? m[c][1] : 0.0;
                out[c] = cv::Vec2d(val, val);
            } else {
                const bool real_ok = (abs_real >= t_min) && (abs_real <= t_max_real);
                const double val_real = real_ok ? m[c][0] : 0.0;
                const double val_imag = imag_ok ? m[c][1] : 0.0;
                out[c] = cv::Vec2d(val_real, val_imag);
            }
        }
    }

    return refined;
}

// ---------------------------------------------------------------------------
// run
// ---------------------------------------------------------------------------
Result<IrisFilterResponse> FragileBitRefinement::run(
    const IrisFilterResponse& input) const {
    if (input.responses.empty()) {
        return make_error(ErrorCode::EncodingFailed,
                          "No filter responses to refine");
    }

    IrisFilterResponse output;
    output.iris_code_version = input.iris_code_version;

    const bool polar = (params_.fragile_type == "polar");

    for (const auto& resp : input.responses) {
        cv::Mat refined_mask = polar
            ? refine_polar(resp.data, resp.mask)
            : refine_cartesian(resp.data, resp.mask);

        output.responses.push_back({resp.data.clone(), refined_mask});
    }

    return output;
}

IRIS_REGISTER_NODE("iris.FragileBitRefinement", FragileBitRefinement);

// Long-form alias matching Python pipeline.yaml class path
IRIS_REGISTER_NODE_ALIAS(
    "iris.nodes.iris_response_refinement.fragile_bits_refinement.FragileBitRefinement",
    FragileBitRefinement, FragileBitRefinement_long);

}  // namespace iris
