#pragma once

#include <string_view>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Binarize IrisFilterResponse to generate iris template using Daugman's method.
///
/// For each filter response:
///   iris_code[..., 0] = (response.real > 0)
///   iris_code[..., 1] = (response.imag > 0)
///   mask_code[..., 0] = (mask.real >= mask_threshold)
///   mask_code[..., 1] = (mask.imag >= mask_threshold)
///
/// Reference: Daugman, "How iris recognition works", IEEE PAMI 2004.
class IrisEncoder : public Algorithm<IrisEncoder> {
public:
    static constexpr std::string_view kName = "IrisEncoder";

    struct Params {
        double mask_threshold = 0.93;
    };

    IrisEncoder() = default;
    explicit IrisEncoder(const Params& params) : params_(params) {}

    /// Construct from generic node params (YAML key-value map).
    explicit IrisEncoder(const std::unordered_map<std::string, std::string>& node_params);

    /// Encode filter responses into an IrisTemplate.
    Result<IrisTemplate> run(const IrisFilterResponse& response) const;

private:
    Params params_;
};

}  // namespace iris
