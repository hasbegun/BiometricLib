#pragma once

#include <string_view>
#include <unordered_map>
#include <vector>

#include <opencv2/core.hpp>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Iris normalization using nonlinear (squared) radial sampling.
class NonlinearNormalization : public Algorithm<NonlinearNormalization> {
public:
    static constexpr std::string_view kName = "NonlinearNormalization";

    struct Params {
        int res_in_r = 128;
        int oversat_threshold = 254;
    };

    NonlinearNormalization() { precompute_grids(); }
    explicit NonlinearNormalization(const Params& params);
    explicit NonlinearNormalization(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<NormalizedIris> run(const IRImage& image,
                               const NoiseMask& noise_mask,
                               const GeometryPolygons& extrapolated_contours,
                               const EyeOrientation& eye_orientation) const;

private:
    Params params_;
    /// Precomputed grids: intermediate_radiuses_[p2i_percent][r_index]
    /// 101 entries (0..100), each with res_in_r doubles in [0,1].
    std::vector<std::vector<double>> intermediate_radiuses_;

    void precompute_grids();
    static std::vector<double> compute_grid(int res_in_r, int p2i_ratio_percent);

    std::vector<std::vector<cv::Point>> generate_correspondences(
        const Contour& pupil_points, const Contour& iris_points,
        double p2i_ratio) const;
};

}  // namespace iris
