#pragma once

#include <vector>

#include <opencv2/core.hpp>

#include <iris/core/error.hpp>
#include <iris/core/types.hpp>
#include <iris/nodes/hamming_distance_matcher.hpp>

namespace iris {

/// Result of N-vs-N matching with rotation information.
struct PairwiseResult {
    DistanceMatrix distances;
    cv::Mat rotations;  // CV_32SC1 (N x N), best rotation per pair
};

/// Batch matching utility (not a pipeline node).
///
/// Wraps HammingDistanceMatcher to provide 1-vs-N and N-vs-N matching
/// with parallel execution via the global thread pool.
class BatchMatcher {
public:
    using Params = HammingDistanceMatcher::Params;

    BatchMatcher() = default;
    explicit BatchMatcher(const Params& params) : matcher_(params) {}

    /// Match one probe against N gallery templates in parallel.
    Result<std::vector<MatchResult>> match_one_vs_n(
        const IrisTemplate& probe,
        const std::vector<IrisTemplate>& gallery) const;

    /// Compute pairwise distance matrix for N templates.
    /// Only computes upper triangle (i < j), mirrors to lower triangle.
    Result<DistanceMatrix> match_n_vs_n(
        const std::vector<IrisTemplate>& templates) const;

    /// Like match_n_vs_n but also returns best rotation per pair.
    Result<PairwiseResult> match_n_vs_n_with_rotations(
        const std::vector<IrisTemplate>& templates) const;

    const Params& params() const { return matcher_.params(); }

private:
    HammingDistanceMatcher matcher_;
};

}  // namespace iris
