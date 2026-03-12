#include <iris/nodes/encoder.hpp>

#include <iris/core/node_registry.hpp>

namespace iris {

IrisEncoder::IrisEncoder(const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("mask_threshold");
    if (it != node_params.end()) {
        params_.mask_threshold = std::stod(it->second);
    }
}

Result<IrisTemplate> IrisEncoder::run(const IrisFilterResponse& response) const {
    if (response.responses.empty()) {
        return make_error(ErrorCode::EncodingFailed, "No filter responses to encode");
    }

    IrisTemplate tmpl;
    tmpl.iris_code_version = response.iris_code_version;

    for (const auto& resp : response.responses) {
        // resp.data is CV_64FC2 (real + imaginary), shape (n_rows, n_cols)
        // resp.mask is CV_64FC2 (real + imaginary mask response)
        const int rows = resp.data.rows;
        const int cols = resp.data.cols;

        // Binarize iris code: 2 bits per position (real > 0, imag > 0)
        // Output shape: (rows, cols * 2) with one byte per bit
        cv::Mat code(rows, cols * 2, CV_8UC1, cv::Scalar(0));
        cv::Mat mask(rows, cols * 2, CV_8UC1, cv::Scalar(0));

        for (int r = 0; r < rows; ++r) {
            const auto* data_row = resp.data.ptr<cv::Vec2d>(r);
            const auto* mask_row = resp.mask.ptr<cv::Vec2d>(r);
            auto* code_row = code.ptr<uint8_t>(r);
            auto* mask_row_out = mask.ptr<uint8_t>(r);

            for (int c = 0; c < cols; ++c) {
                // Bit 0: real part
                code_row[c * 2] = data_row[c][0] > 0.0 ? 1 : 0;
                // Bit 1: imaginary part
                code_row[c * 2 + 1] = data_row[c][1] > 0.0 ? 1 : 0;

                // Mask: real >= threshold, imag >= threshold
                mask_row_out[c * 2] = mask_row[c][0] >= params_.mask_threshold ? 1 : 0;
                mask_row_out[c * 2 + 1] = mask_row[c][1] >= params_.mask_threshold ? 1 : 0;
            }
        }

        // Pack into PackedIrisCode
        tmpl.iris_codes.push_back(PackedIrisCode::from_mat(code, mask));
        tmpl.mask_codes.push_back(PackedIrisCode::from_mat(mask, mask));
    }

    return tmpl;
}

// Register with the node registry under the Python class name
IRIS_REGISTER_NODE("iris.IrisEncoder", IrisEncoder);

}  // namespace iris
