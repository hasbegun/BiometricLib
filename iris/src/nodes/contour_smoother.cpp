#include <iris/nodes/contour_smoother.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <numeric>
#include <vector>

#include <iris/core/node_registry.hpp>
#include <iris/utils/geometry_utils.hpp>

namespace iris {

ContourSmoother::ContourSmoother(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto get = [&](const std::string& key, double& out) {
        auto it = node_params.find(key);
        if (it != node_params.end()) {
            out = std::stod(it->second);
        }
    };
    get("dphi", params_.dphi);
    get("kernel_size", params_.kernel_size);
    get("gap_threshold", params_.gap_threshold);
}

namespace {

constexpr double kTwoPi = 2.0 * std::numbers::pi;
constexpr double kDegToRad = std::numbers::pi / 180.0;

/// Sort two arrays together based on the first array's order.
void sort_by_phi(std::vector<double>& phis, std::vector<double>& rhos) {
    std::vector<size_t> idx(phis.size());
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(),
              [&](size_t a, size_t b) { return phis[a] < phis[b]; });

    std::vector<double> sorted_phis(phis.size());
    std::vector<double> sorted_rhos(rhos.size());
    for (size_t i = 0; i < idx.size(); ++i) {
        sorted_phis[i] = phis[idx[i]];
        sorted_rhos[i] = rhos[idx[i]];
    }
    phis = std::move(sorted_phis);
    rhos = std::move(sorted_rhos);
}

/// 1D linear interpolation at uniform phi intervals.
/// Supports periodic boundary via period parameter (0 = no period).
std::pair<std::vector<double>, std::vector<double>>
interp_uniform(const std::vector<double>& phis,
               const std::vector<double>& rhos,
               double dphi_rad, double period) {
    if (phis.empty()) return {{}, {}};

    const double phi_min = phis.front();
    const double phi_max = phis.back();

    // Create uniform grid
    std::vector<double> out_phi;
    for (double p = phi_min; p < phi_max; p += dphi_rad) {
        out_phi.push_back(p);
    }

    std::vector<double> out_rho(out_phi.size());
    for (size_t i = 0; i < out_phi.size(); ++i) {
        double target = out_phi[i];

        // Binary search for interval
        auto it = std::lower_bound(phis.begin(), phis.end(), target);
        if (it == phis.begin()) {
            out_rho[i] = rhos.front();
        } else if (it == phis.end()) {
            out_rho[i] = rhos.back();
        } else {
            auto j = static_cast<size_t>(it - phis.begin());
            double t = (target - phis[j - 1]) / (phis[j] - phis[j - 1]);
            out_rho[i] = rhos[j - 1] + t * (rhos[j] - rhos[j - 1]);
        }
    }

    return {std::move(out_phi), std::move(out_rho)};
}

/// Rolling median filter on a 1D signal. Trims kernel_offset from each end.
std::vector<double> rolling_median(const std::vector<double>& signal,
                                    int kernel_offset) {
    const auto n = static_cast<int>(signal.size());
    if (n == 0 || kernel_offset <= 0) return signal;

    const int kernel_size = 2 * kernel_offset + 1;
    const int out_start = kernel_offset;
    const int out_end = n - kernel_offset;

    if (out_end <= out_start) return signal;

    std::vector<double> result;
    result.reserve(static_cast<size_t>(out_end - out_start));

    std::vector<double> window(static_cast<size_t>(kernel_size));

    for (int i = out_start; i < out_end; ++i) {
        for (int k = -kernel_offset; k <= kernel_offset; ++k) {
            // Circular boundary wrapping
            int idx = (i + k + n) % n;
            window[static_cast<size_t>(k + kernel_offset)] =
                signal[static_cast<size_t>(idx)];
        }
        std::nth_element(window.begin(),
                         window.begin() + kernel_offset,
                         window.end());
        result.push_back(window[static_cast<size_t>(kernel_offset)]);
    }

    return result;
}

/// Core smoothing: interpolate at uniform dphi, then rolling median.
/// Returns (smoothed_phi, smoothed_rho).
std::pair<std::vector<double>, std::vector<double>>
smooth_array(const std::vector<double>& phis,
             const std::vector<double>& rhos,
             double dphi_rad, int kernel_offset) {
    auto [interp_phi, interp_rho] =
        interp_uniform(phis, rhos, dphi_rad, kTwoPi);

    if (interp_rho.size() < 3) {
        return {interp_phi, interp_rho};
    }

    auto smoothed_rho = rolling_median(interp_rho, kernel_offset);

    // Trim phi to match smoothed_rho
    std::vector<double> smoothed_phi;
    if (static_cast<int>(interp_phi.size()) - 1 >= kernel_offset * 2) {
        smoothed_phi.assign(
            interp_phi.begin() + kernel_offset,
            interp_phi.end() - kernel_offset);
    } else {
        smoothed_phi = interp_phi;
    }

    // Ensure sizes match
    auto min_size = std::min(smoothed_phi.size(), smoothed_rho.size());
    smoothed_phi.resize(min_size);
    smoothed_rho.resize(min_size);

    return {std::move(smoothed_phi), std::move(smoothed_rho)};
}

/// Smooth a complete circular contour (no gaps).
Contour smooth_circular(const Contour& contour, double cx, double cy,
                         double dphi_rad, int kernel_offset) {
    auto [rhos, phis] = cartesian2polar(contour, cx, cy);
    sort_by_phi(phis, rhos);

    // Pad with 3 copies for circular boundary handling
    std::vector<double> padded_phi;
    std::vector<double> padded_rho;
    padded_phi.reserve(phis.size() * 3);
    padded_rho.reserve(rhos.size() * 3);

    for (size_t i = 0; i < phis.size(); ++i) {
        padded_phi.push_back(phis[i] - kTwoPi);
        padded_rho.push_back(rhos[i]);
    }
    for (size_t i = 0; i < phis.size(); ++i) {
        padded_phi.push_back(phis[i]);
        padded_rho.push_back(rhos[i]);
    }
    for (size_t i = 0; i < phis.size(); ++i) {
        padded_phi.push_back(phis[i] + kTwoPi);
        padded_rho.push_back(rhos[i]);
    }

    auto [sm_phi, sm_rho] =
        smooth_array(padded_phi, padded_rho, dphi_rad, kernel_offset);

    // Extract central portion [0, 2*pi)
    std::vector<double> out_phi, out_rho;
    for (size_t i = 0; i < sm_phi.size(); ++i) {
        if (sm_phi[i] >= 0.0 && sm_phi[i] < kTwoPi) {
            out_phi.push_back(sm_phi[i]);
            out_rho.push_back(sm_rho[i]);
        }
    }

    return polar2contour(out_rho, out_phi, cx, cy);
}

/// Find the index of the largest gap in sorted phi values.
size_t find_start_index(const std::vector<double>& phi) {
    if (phi.size() < 2) return 0;

    double max_gap = 0.0;
    size_t max_idx = 0;

    for (size_t i = 1; i < phi.size(); ++i) {
        double gap = phi[i] - phi[i - 1];
        if (gap > max_gap) {
            max_gap = gap;
            max_idx = i;
        }
    }

    // Also check wrap-around gap
    double wrap_gap = kTwoPi - phi.back() + phi.front();
    if (wrap_gap > max_gap) {
        max_idx = 0;
    }

    return max_idx;
}

/// Smooth a single arc (partial contour).
Contour smooth_arc(const Contour& contour, double cx, double cy,
                    double dphi_rad, int kernel_offset) {
    auto [rhos, phis] = cartesian2polar(contour, cx, cy);
    sort_by_phi(phis, rhos);

    // Find largest gap and unwrap
    size_t start_idx = find_start_index(phis);
    double offset = phis[start_idx];

    // Create relative phi coordinates
    std::vector<double> rel_phi(phis.size());
    for (size_t i = 0; i < phis.size(); ++i) {
        rel_phi[i] = std::fmod(phis[i] - offset + kTwoPi, kTwoPi);
    }

    // Re-sort by relative phi
    sort_by_phi(rel_phi, rhos);

    auto [sm_phi, sm_rho] =
        smooth_array(rel_phi, rhos, dphi_rad, kernel_offset);

    // Wrap back to absolute coordinates
    for (auto& p : sm_phi) {
        p = std::fmod(p + offset, kTwoPi);
    }

    return polar2contour(sm_rho, sm_phi, cx, cy);
}

/// Detect gaps in a contour and split into arcs.
struct ArcSplit {
    std::vector<Contour> arcs;
    int num_gaps;
};

ArcSplit cut_into_arcs(const Contour& contour, double cx, double cy,
                        double gap_threshold_rad) {
    auto [rhos, phis] = cartesian2polar(contour, cx, cy);
    sort_by_phi(phis, rhos);

    const auto n = phis.size();
    if (n < 2) {
        return {{contour}, 0};
    }

    // Compute angular differences
    std::vector<double> diffs(n);
    for (size_t i = 0; i < n - 1; ++i) {
        diffs[i] = std::abs(phis[i + 1] - phis[i]);
    }
    // Wrap-around difference
    diffs[n - 1] = kTwoPi - (phis[n - 1] - phis[0]);

    // Find gap indices
    std::vector<size_t> gap_indices;
    for (size_t i = 0; i < n; ++i) {
        if (diffs[i] > gap_threshold_rad) {
            gap_indices.push_back(i);
        }
    }

    if (gap_indices.size() < 2) {
        return {{contour}, static_cast<int>(gap_indices.size())};
    }

    // Split at gaps (shift by +1 for split positions)
    std::vector<size_t> split_points;
    for (auto idx : gap_indices) {
        split_points.push_back(idx + 1);
    }

    // Create arc segments from sorted polar coordinates
    std::vector<Contour> arcs;
    size_t prev = 0;
    for (auto sp : split_points) {
        if (sp > n) sp = n;
        if (sp <= prev) continue;

        std::vector<double> arc_rho(rhos.begin() +
                                        static_cast<ptrdiff_t>(prev),
                                    rhos.begin() +
                                        static_cast<ptrdiff_t>(sp));
        std::vector<double> arc_phi(phis.begin() +
                                        static_cast<ptrdiff_t>(prev),
                                    phis.begin() +
                                        static_cast<ptrdiff_t>(sp));
        arcs.push_back(polar2contour(arc_rho, arc_phi, cx, cy));
        prev = sp;
    }

    // Remaining points
    if (prev < n) {
        std::vector<double> arc_rho(rhos.begin() +
                                        static_cast<ptrdiff_t>(prev),
                                    rhos.end());
        std::vector<double> arc_phi(phis.begin() +
                                        static_cast<ptrdiff_t>(prev),
                                    phis.end());
        auto last_arc = polar2contour(arc_rho, arc_phi, cx, cy);

        // Merge with first arc (wraparound connection)
        if (!arcs.empty()) {
            arcs[0].points.insert(arcs[0].points.begin(),
                                   last_arc.points.begin(),
                                   last_arc.points.end());
        } else {
            arcs.push_back(std::move(last_arc));
        }
    }

    return {std::move(arcs), static_cast<int>(gap_indices.size())};
}

/// Smooth a single contour (dispatch circular vs arc-based).
Contour smooth_contour(const Contour& contour, double cx, double cy,
                        double dphi_rad, int kernel_offset,
                        double gap_threshold_rad) {
    if (contour.points.size() < 3) {
        return contour;
    }

    auto [arcs, num_gaps] = cut_into_arcs(contour, cx, cy, gap_threshold_rad);

    if (num_gaps == 0) {
        return smooth_circular(contour, cx, cy, dphi_rad, kernel_offset);
    }

    // Smooth each arc separately and concatenate
    Contour result;
    for (const auto& arc : arcs) {
        if (arc.points.size() < 2) continue;
        auto smoothed = smooth_arc(arc, cx, cy, dphi_rad, kernel_offset);
        result.points.insert(result.points.end(),
                              smoothed.points.begin(),
                              smoothed.points.end());
    }

    return result;
}

}  // namespace

Result<GeometryPolygons> ContourSmoother::run(
    const GeometryPolygons& polygons,
    const EyeCenters& eye_centers) const {
    if (polygons.iris.points.empty()) {
        return make_error(ErrorCode::GeometryEstimationFailed,
                          "Empty iris contour — cannot smooth");
    }

    const double dphi_rad = params_.dphi * kDegToRad;
    const double gap_rad = params_.gap_threshold * kDegToRad;
    const int kernel_offset = std::max(
        1, static_cast<int>(
               (params_.kernel_size * kDegToRad) / dphi_rad) / 2);

    GeometryPolygons result;

    result.iris = smooth_contour(
        polygons.iris, eye_centers.iris_center.x, eye_centers.iris_center.y,
        dphi_rad, kernel_offset, gap_rad);

    result.pupil = smooth_contour(
        polygons.pupil, eye_centers.pupil_center.x, eye_centers.pupil_center.y,
        dphi_rad, kernel_offset, gap_rad);

    // Eyeball passes through unchanged
    result.eyeball = polygons.eyeball;

    return result;
}

IRIS_REGISTER_NODE("iris.Smoothing", ContourSmoother);

}  // namespace iris
