#pragma once

#include <array>
#include <string>
#include <string_view>
#include <unordered_map>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Refines filter response masks by thresholding fragile bits.
///
/// Fragile bits are response positions where the filter output is close
/// to zero (ambiguous sign). This node zeros out the corresponding mask
/// entries so those bits are ignored during matching.
///
/// Supports polar and cartesian thresholding modes.
class FragileBitRefinement : public Algorithm<FragileBitRefinement> {
public:
    static constexpr std::string_view kName = "FragileBitRefinement";

    struct Params {
        std::array<double, 3> value_threshold = {0.0001, 0.275, 0.0873};
        std::string fragile_type = "polar";   // "polar" or "cartesian"
        bool mask_is_duplicated = false;
    };

    FragileBitRefinement() = default;
    explicit FragileBitRefinement(const Params& params) : params_(params) {}
    explicit FragileBitRefinement(
        const std::unordered_map<std::string, std::string>& node_params);

    /// Refine masks in the filter response. Data is passed through unchanged.
    Result<IrisFilterResponse> run(const IrisFilterResponse& input) const;

    const Params& params() const noexcept { return params_; }

private:
    Params params_;

    /// Refine one response's mask using polar thresholds.
    cv::Mat refine_polar(const cv::Mat& data, const cv::Mat& mask) const;

    /// Refine one response's mask using cartesian thresholds.
    cv::Mat refine_cartesian(const cv::Mat& data, const cv::Mat& mask) const;
};

}  // namespace iris
