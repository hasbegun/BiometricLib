#include <iris/nodes/linear_extrapolator.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <numeric>
#include <vector>

#include <iris/core/node_registry.hpp>
#include <iris/utils/geometry_utils.hpp>

namespace iris {

LinearExtrapolator::LinearExtrapolator(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("dphi");
    if (it != node_params.end()) {
        params_.dphi = std::stod(it->second);
    }
}

namespace {

constexpr double kTwoPi = 2.0 * std::numbers::pi;
constexpr double kDegToRad = std::numbers::pi / 180.0;

/// Estimate a contour by linear interpolation in polar space.
Contour estimate(const Contour& contour, double cx, double cy,
                  double dphi_rad) {
    auto [rhos, phis] = cartesian2polar(contour, cx, cy);

    // Sort by phi
    std::vector<size_t> idx(phis.size());
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(),
              [&](size_t a, size_t b) { return phis[a] < phis[b]; });

    std::vector<double> sorted_phi(phis.size());
    std::vector<double> sorted_rho(rhos.size());
    for (size_t i = 0; i < idx.size(); ++i) {
        sorted_phi[i] = phis[idx[i]];
        sorted_rho[i] = rhos[idx[i]];
    }

    // Pad with 3 copies for periodic boundary
    std::vector<double> padded_phi;
    std::vector<double> padded_rho;
    padded_phi.reserve(sorted_phi.size() * 3);
    padded_rho.reserve(sorted_rho.size() * 3);

    for (size_t i = 0; i < sorted_phi.size(); ++i) {
        padded_phi.push_back(sorted_phi[i] - kTwoPi);
        padded_rho.push_back(sorted_rho[i]);
    }
    for (size_t i = 0; i < sorted_phi.size(); ++i) {
        padded_phi.push_back(sorted_phi[i]);
        padded_rho.push_back(sorted_rho[i]);
    }
    for (size_t i = 0; i < sorted_phi.size(); ++i) {
        padded_phi.push_back(sorted_phi[i] + kTwoPi);
        padded_rho.push_back(sorted_rho[i]);
    }

    // Create uniform angular grid over the padded range
    std::vector<double> interp_phi;
    for (double p = padded_phi.front(); p < padded_phi.back(); p += dphi_rad) {
        interp_phi.push_back(p);
    }

    // Interpolate rho values
    std::vector<double> interp_rho(interp_phi.size());
    for (size_t i = 0; i < interp_phi.size(); ++i) {
        double target = interp_phi[i];
        auto it = std::lower_bound(padded_phi.begin(), padded_phi.end(), target);
        if (it == padded_phi.begin()) {
            interp_rho[i] = padded_rho.front();
        } else if (it == padded_phi.end()) {
            interp_rho[i] = padded_rho.back();
        } else {
            auto j = static_cast<size_t>(it - padded_phi.begin());
            double t = (target - padded_phi[j - 1]) /
                       (padded_phi[j] - padded_phi[j - 1]);
            interp_rho[i] =
                padded_rho[j - 1] + t * (padded_rho[j] - padded_rho[j - 1]);
        }
    }

    // Extract only points in [0, 2*pi)
    std::vector<double> out_phi, out_rho;
    for (size_t i = 0; i < interp_phi.size(); ++i) {
        if (interp_phi[i] >= 0.0 && interp_phi[i] < kTwoPi) {
            out_phi.push_back(interp_phi[i]);
            out_rho.push_back(interp_rho[i]);
        }
    }

    return polar2contour(out_rho, out_phi, cx, cy);
}

}  // namespace

Result<GeometryPolygons> LinearExtrapolator::run(
    const GeometryPolygons& polygons,
    const EyeCenters& eye_centers) const {
    if (polygons.iris.points.size() < 3) {
        return make_error(ErrorCode::GeometryEstimationFailed,
                          "Iris contour has fewer than 3 points");
    }

    const double dphi_rad = params_.dphi * kDegToRad;

    GeometryPolygons result;

    result.iris = estimate(polygons.iris,
                            eye_centers.iris_center.x,
                            eye_centers.iris_center.y,
                            dphi_rad);

    if (polygons.pupil.points.size() >= 3) {
        result.pupil = estimate(polygons.pupil,
                                 eye_centers.pupil_center.x,
                                 eye_centers.pupil_center.y,
                                 dphi_rad);
    } else {
        result.pupil = polygons.pupil;
    }

    // Eyeball passes through unchanged
    result.eyeball = polygons.eyeball;

    return result;
}

IRIS_REGISTER_NODE("iris.LinearExtrapolation", LinearExtrapolator);

}  // namespace iris
