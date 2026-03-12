#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>

namespace iris {

/// Regular grid probe schema for sampling normalized iris images.
/// Generates uniform (rho, phi) coordinates for feature extraction.
class RegularProbeSchema : public Algorithm<RegularProbeSchema> {
public:
    static constexpr std::string_view kName = "RegularProbeSchema";

    struct Params {
        int n_rows = 16;
        int n_cols = 256;
        double boundary_rho_low = 0.0;
        double boundary_rho_high = 0.0625;
        std::string boundary_phi = "periodic-left";
    };

    RegularProbeSchema() { generate_schema(); }
    explicit RegularProbeSchema(const Params& params)
        : params_(params) { generate_schema(); }
    explicit RegularProbeSchema(
        const std::unordered_map<std::string, std::string>& node_params);

    const std::vector<double>& rhos() const noexcept { return rhos_; }
    const std::vector<double>& phis() const noexcept { return phis_; }
    const Params& params() const noexcept { return params_; }

private:
    Params params_;
    std::vector<double> rhos_;  // flattened, size = n_rows * n_cols
    std::vector<double> phis_;  // flattened, size = n_rows * n_cols

    void generate_schema();
};

}  // namespace iris
