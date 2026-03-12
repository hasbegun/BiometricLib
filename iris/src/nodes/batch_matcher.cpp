#include <iris/nodes/batch_matcher.hpp>

#include <iris/core/thread_pool.hpp>

namespace iris {

Result<std::vector<MatchResult>> BatchMatcher::match_one_vs_n(
    const IrisTemplate& probe,
    const std::vector<IrisTemplate>& gallery) const {

    if (gallery.empty()) {
        return std::vector<MatchResult>{};
    }

    const size_t n = gallery.size();
    std::vector<MatchResult> results(n);
    std::optional<IrisError> first_error;
    std::mutex error_mutex;

    global_pool().parallel_for(0, n, [&](size_t i) {
        auto r = matcher_.run(probe, gallery[i]);
        if (r.has_value()) {
            results[i] = r.value();
        } else {
            std::lock_guard lock(error_mutex);
            if (!first_error) first_error = r.error();
        }
    });

    if (first_error) {
        return std::unexpected(*first_error);
    }
    return results;
}

Result<DistanceMatrix> BatchMatcher::match_n_vs_n(
    const std::vector<IrisTemplate>& templates) const {

    const size_t n = templates.size();
    DistanceMatrix dm(n);

    if (n <= 1) return dm;

    // Compute upper triangle: total pairs = n*(n-1)/2
    const size_t n_pairs = n * (n - 1) / 2;
    std::optional<IrisError> first_error;
    std::mutex error_mutex;

    global_pool().parallel_for(0, n_pairs, [&](size_t idx) {
        // Map linear index to (i, j) where i < j
        // idx = i * n - i*(i+1)/2 + j - i - 1
        // Use a simpler mapping: iterate row by row
        size_t i = 0, j = 0;
        {
            size_t remaining = idx;
            for (i = 0; i < n - 1; ++i) {
                const size_t row_len = n - i - 1;
                if (remaining < row_len) {
                    j = i + 1 + remaining;
                    break;
                }
                remaining -= row_len;
            }
        }

        auto r = matcher_.run(templates[i], templates[j]);
        if (r.has_value()) {
            dm(i, j) = r.value().distance;
            dm(j, i) = r.value().distance;
        } else {
            std::lock_guard lock(error_mutex);
            if (!first_error) first_error = r.error();
        }
    });

    if (first_error) {
        return std::unexpected(*first_error);
    }
    return dm;
}

Result<PairwiseResult> BatchMatcher::match_n_vs_n_with_rotations(
    const std::vector<IrisTemplate>& templates) const {

    const size_t n = templates.size();
    const auto n_int = static_cast<int>(n);
    PairwiseResult pr{
        .distances = DistanceMatrix(n),
        .rotations = cv::Mat::zeros(n_int, n_int, CV_32SC1)
    };

    if (n <= 1) return pr;

    const size_t n_pairs = n * (n - 1) / 2;
    std::optional<IrisError> first_error;
    std::mutex error_mutex;

    global_pool().parallel_for(0, n_pairs, [&](size_t idx) {
        size_t i = 0, j = 0;
        {
            size_t remaining = idx;
            for (i = 0; i < n - 1; ++i) {
                const size_t row_len = n - i - 1;
                if (remaining < row_len) {
                    j = i + 1 + remaining;
                    break;
                }
                remaining -= row_len;
            }
        }

        auto r = matcher_.run(templates[i], templates[j]);
        if (r.has_value()) {
            pr.distances(i, j) = r.value().distance;
            pr.distances(j, i) = r.value().distance;
            pr.rotations.at<int32_t>(static_cast<int>(i), static_cast<int>(j))
                = r.value().best_rotation;
            pr.rotations.at<int32_t>(static_cast<int>(j), static_cast<int>(i))
                = -r.value().best_rotation;
        } else {
            std::lock_guard lock(error_mutex);
            if (!first_error) first_error = r.error();
        }
    });

    if (first_error) {
        return std::unexpected(*first_error);
    }
    return pr;
}

}  // namespace iris
