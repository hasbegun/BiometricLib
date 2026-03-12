#include <iris/nodes/hamming_distance_matcher.hpp>

#include <algorithm>
#include <cmath>

#include <iris/core/node_registry.hpp>
#include <iris/nodes/matcher_simd.hpp>

namespace iris {

// --- Helpers ---

namespace {

/// Compute combined mask = AND(probe_mask, gallery_mask) for a word range.
/// Returns {popcount(code_xor AND mask), popcount(mask)}.
struct BitCounts {
    size_t iris_bits = 0;
    size_t mask_bits = 0;
};

BitCounts count_bits(
    const PackedIrisCode& rotated_probe,
    const PackedIrisCode& gallery_code,
    size_t word_begin, size_t word_end) {

    const size_t n = word_end - word_begin;

    // Build combined mask for this word range
    alignas(64) uint64_t combined_mask[PackedIrisCode::kNumWords]{};
    for (size_t w = 0; w < n; ++w) {
        combined_mask[w] = rotated_probe.mask_bits[word_begin + w]
                         & gallery_code.mask_bits[word_begin + w];
    }

    BitCounts bc;
    bc.iris_bits = simd::popcount_xor_and(
        rotated_probe.code_bits.data() + word_begin,
        gallery_code.code_bits.data() + word_begin,
        combined_mask, n);
    bc.mask_bits = simd::popcount_and(combined_mask, combined_mask, n);
    return bc;
}

double normalize_hd(double raw_hd, size_t mask_bits, double norm_mean, double norm_gradient) {
    return std::max(0.0,
        norm_mean - (norm_mean - raw_hd)
                  * (norm_gradient * static_cast<double>(mask_bits) + 0.5));
}

}  // namespace

// --- SimpleHammingDistanceMatcher ---

SimpleHammingDistanceMatcher::SimpleHammingDistanceMatcher(
    const std::unordered_map<std::string, std::string>& node_params) {
    if (auto it = node_params.find("rotation_shift"); it != node_params.end())
        params_.rotation_shift = std::stoi(it->second);
    if (auto it = node_params.find("normalise"); it != node_params.end())
        params_.normalise = (it->second == "true" || it->second == "1");
    if (auto it = node_params.find("norm_mean"); it != node_params.end())
        params_.norm_mean = std::stod(it->second);
    if (auto it = node_params.find("norm_gradient"); it != node_params.end())
        params_.norm_gradient = std::stod(it->second);
}

Result<MatchResult> SimpleHammingDistanceMatcher::run(
    const IrisTemplate& probe, const IrisTemplate& gallery) const {

    if (probe.iris_codes.empty() || gallery.iris_codes.empty()) {
        return make_error(ErrorCode::MatchingFailed, "Empty iris codes");
    }
    if (probe.iris_codes.size() != gallery.iris_codes.size()) {
        return make_error(ErrorCode::MatchingFailed,
            "Mismatched number of iris code pairs");
    }

    const size_t n_pairs = probe.iris_codes.size();
    MatchResult best{.distance = 1.0, .best_rotation = 0};

    for (int abs_shift = 0; abs_shift <= params_.rotation_shift; ++abs_shift) {
        for (int sign : {1, -1}) {
            if (abs_shift == 0 && sign == -1) continue;
            const int shift = abs_shift * sign;

            size_t total_iris = 0;
            size_t total_mask = 0;

            for (size_t i = 0; i < n_pairs; ++i) {
                auto rotated = probe.iris_codes[i].rotate(shift);
                auto bc = count_bits(rotated, gallery.iris_codes[i],
                                     0, PackedIrisCode::kNumWords);
                total_iris += bc.iris_bits;
                total_mask += bc.mask_bits;
            }

            if (total_mask == 0) continue;

            double hd = static_cast<double>(total_iris)
                      / static_cast<double>(total_mask);

            if (params_.normalise) {
                hd = normalize_hd(hd, total_mask, params_.norm_mean, params_.norm_gradient);
            }

            if (hd < best.distance) {
                best.distance = hd;
                best.best_rotation = shift;
            }
        }
    }

    return best;
}

// --- HammingDistanceMatcher ---

HammingDistanceMatcher::HammingDistanceMatcher(
    const std::unordered_map<std::string, std::string>& node_params) {
    if (auto it = node_params.find("rotation_shift"); it != node_params.end())
        params_.rotation_shift = std::stoi(it->second);
    if (auto it = node_params.find("normalise"); it != node_params.end())
        params_.normalise = (it->second == "true" || it->second == "1");
    if (auto it = node_params.find("norm_mean"); it != node_params.end())
        params_.norm_mean = std::stod(it->second);
    if (auto it = node_params.find("norm_gradient"); it != node_params.end())
        params_.norm_gradient = std::stod(it->second);
    if (auto it = node_params.find("separate_half_matching"); it != node_params.end())
        params_.separate_half_matching = (it->second == "true" || it->second == "1");
}

Result<MatchResult> HammingDistanceMatcher::run(
    const IrisTemplate& probe, const IrisTemplate& gallery) const {

    if (probe.iris_codes.empty() || gallery.iris_codes.empty()) {
        return make_error(ErrorCode::MatchingFailed, "Empty iris codes");
    }
    if (probe.iris_codes.size() != gallery.iris_codes.size()) {
        return make_error(ErrorCode::MatchingFailed,
            "Mismatched number of iris code pairs");
    }

    const size_t n_pairs = probe.iris_codes.size();
    constexpr size_t kHalfWords = PackedIrisCode::kNumWords / 2;  // 64

    MatchResult best{.distance = 1.0, .best_rotation = 0};

    for (int abs_shift = 0; abs_shift <= params_.rotation_shift; ++abs_shift) {
        for (int sign : {1, -1}) {
            if (abs_shift == 0 && sign == -1) continue;
            const int shift = abs_shift * sign;

            double hd;

            if (params_.separate_half_matching) {
                // Compute upper and lower halves independently
                size_t upper_iris = 0, upper_mask = 0;
                size_t lower_iris = 0, lower_mask = 0;

                for (size_t i = 0; i < n_pairs; ++i) {
                    auto rotated = probe.iris_codes[i].rotate(shift);

                    auto upper = count_bits(rotated, gallery.iris_codes[i],
                                            0, kHalfWords);
                    auto lower = count_bits(rotated, gallery.iris_codes[i],
                                            kHalfWords, PackedIrisCode::kNumWords);

                    upper_iris += upper.iris_bits;
                    upper_mask += upper.mask_bits;
                    lower_iris += lower.iris_bits;
                    lower_mask += lower.mask_bits;
                }

                // Average the two halves (skip any half with zero mask)
                double sum = 0.0;
                int count = 0;

                if (upper_mask > 0) {
                    double upper_hd = static_cast<double>(upper_iris)
                                    / static_cast<double>(upper_mask);
                    if (params_.normalise) {
                        upper_hd = normalize_hd(upper_hd, upper_mask,
                                                params_.norm_mean, params_.norm_gradient);
                    }
                    sum += upper_hd;
                    ++count;
                }

                if (lower_mask > 0) {
                    double lower_hd = static_cast<double>(lower_iris)
                                    / static_cast<double>(lower_mask);
                    if (params_.normalise) {
                        lower_hd = normalize_hd(lower_hd, lower_mask,
                                                params_.norm_mean, params_.norm_gradient);
                    }
                    sum += lower_hd;
                    ++count;
                }

                if (count == 0) continue;
                hd = sum / static_cast<double>(count);
            } else {
                // Full code matching (same as SimpleHammingDistanceMatcher)
                size_t total_iris = 0;
                size_t total_mask = 0;

                for (size_t i = 0; i < n_pairs; ++i) {
                    auto rotated = probe.iris_codes[i].rotate(shift);
                    auto bc = count_bits(rotated, gallery.iris_codes[i],
                                         0, PackedIrisCode::kNumWords);
                    total_iris += bc.iris_bits;
                    total_mask += bc.mask_bits;
                }

                if (total_mask == 0) continue;

                hd = static_cast<double>(total_iris)
                   / static_cast<double>(total_mask);

                if (params_.normalise) {
                    hd = normalize_hd(hd, total_mask,
                                      params_.norm_mean, params_.norm_gradient);
                }
            }

            if (hd < best.distance) {
                best.distance = hd;
                best.best_rotation = shift;
            }
        }
    }

    return best;
}

IRIS_REGISTER_NODE("iris.SimpleHammingDistanceMatcher", SimpleHammingDistanceMatcher);
IRIS_REGISTER_NODE("iris.HammingDistanceMatcher", HammingDistanceMatcher);

}  // namespace iris
