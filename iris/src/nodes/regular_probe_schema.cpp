#include <iris/nodes/regular_probe_schema.hpp>

#include <string>

#include <iris/core/node_registry.hpp>

namespace iris {

RegularProbeSchema::RegularProbeSchema(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("n_rows");
    if (it != node_params.end()) params_.n_rows = std::stoi(it->second);

    it = node_params.find("n_cols");
    if (it != node_params.end()) params_.n_cols = std::stoi(it->second);

    it = node_params.find("boundary_rho_low");
    if (it != node_params.end()) params_.boundary_rho_low = std::stod(it->second);

    it = node_params.find("boundary_rho_high");
    if (it != node_params.end()) params_.boundary_rho_high = std::stod(it->second);

    it = node_params.find("boundary_phi");
    if (it != node_params.end()) params_.boundary_phi = it->second;

    generate_schema();
}

void RegularProbeSchema::generate_schema() {
    const int nr = params_.n_rows;
    const int nc = params_.n_cols;
    const auto total = static_cast<size_t>(nr) * static_cast<size_t>(nc);

    rhos_.resize(total);
    phis_.resize(total);

    // Generate rho values: linspace(boundary_low, 1 - boundary_high, n_rows)
    // with endpoint=true
    std::vector<double> rho_1d(static_cast<size_t>(nr));
    {
        const double rho_start = params_.boundary_rho_low;
        const double rho_end = 1.0 - params_.boundary_rho_high;
        const double step = (nr > 1) ? (rho_end - rho_start)
                                         / static_cast<double>(nr - 1)
                                     : 0.0;
        for (int i = 0; i < nr; ++i) {
            rho_1d[static_cast<size_t>(i)] = rho_start
                                             + static_cast<double>(i) * step;
        }
    }

    // Generate phi values: linspace(0, 1, n_cols, endpoint=false)
    std::vector<double> phi_1d(static_cast<size_t>(nc));
    {
        const double spacing = 1.0 / static_cast<double>(nc);
        double offset = 0.0;
        if (params_.boundary_phi == "periodic-symmetric") {
            offset = spacing / 2.0;
        }
        // "periodic-left": offset = 0
        for (int i = 0; i < nc; ++i) {
            phi_1d[static_cast<size_t>(i)] =
                static_cast<double>(i) * spacing + offset;
        }
    }

    // Meshgrid: outer loop rho (rows), inner loop phi (cols) → flatten
    for (int r = 0; r < nr; ++r) {
        for (int c = 0; c < nc; ++c) {
            const auto idx = static_cast<size_t>(r) * static_cast<size_t>(nc)
                             + static_cast<size_t>(c);
            rhos_[idx] = rho_1d[static_cast<size_t>(r)];
            phis_[idx] = phi_1d[static_cast<size_t>(c)];
        }
    }
}

IRIS_REGISTER_NODE("iris.RegularProbeSchema", RegularProbeSchema);

}  // namespace iris
