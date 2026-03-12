#pragma once

#include <string_view>
#include <unordered_map>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Result of a single probe-vs-gallery match.
struct MatchResult {
    double distance = 1.0;
    int best_rotation = 0;
};

/// Simple Hamming distance matcher (no half-splitting, no normalisation by default).
///
/// For each rotation shift in [-rotation_shift, +rotation_shift]:
///   1. Rotate probe code/mask by `shift` columns
///   2. Combined mask = AND(rotated probe mask, gallery mask)
///   3. HD = popcount((probe XOR gallery) AND mask) / popcount(mask)
///   4. Track the minimum across all shifts and wavelet pairs
class SimpleHammingDistanceMatcher : public Algorithm<SimpleHammingDistanceMatcher> {
public:
    static constexpr std::string_view kName = "SimpleHammingDistanceMatcher";

    struct Params {
        int rotation_shift = 15;
        bool normalise = false;
        double norm_mean = 0.45;
        double norm_gradient = 0.00005;
    };

    SimpleHammingDistanceMatcher() = default;
    explicit SimpleHammingDistanceMatcher(const Params& params) : params_(params) {}
    explicit SimpleHammingDistanceMatcher(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<MatchResult> run(const IrisTemplate& probe, const IrisTemplate& gallery) const;

    const Params& params() const { return params_; }

private:
    Params params_;
};

/// Full Hamming distance matcher with separate half matching and normalisation.
///
/// When separate_half_matching is enabled, the iris code is split at row 8
/// (word 64) and upper/lower halves are scored independently, then averaged.
/// This helps when one half of the iris is occluded.
class HammingDistanceMatcher : public Algorithm<HammingDistanceMatcher> {
public:
    static constexpr std::string_view kName = "HammingDistanceMatcher";

    struct Params {
        int rotation_shift = 15;
        bool normalise = true;
        double norm_mean = 0.45;
        double norm_gradient = 0.00005;
        bool separate_half_matching = true;
    };

    HammingDistanceMatcher() = default;
    explicit HammingDistanceMatcher(const Params& params) : params_(params) {}
    explicit HammingDistanceMatcher(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<MatchResult> run(const IrisTemplate& probe, const IrisTemplate& gallery) const;

    const Params& params() const { return params_; }

private:
    Params params_;
};

}  // namespace iris
