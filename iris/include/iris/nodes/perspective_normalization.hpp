#pragma once

#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <opencv2/core.hpp>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Iris normalization using per-ROI perspective transforms.
class PerspectiveNormalization : public Algorithm<PerspectiveNormalization> {
public:
    static constexpr std::string_view kName = "PerspectiveNormalization";

    struct Params {
        int res_in_phi = 1024;
        int res_in_r = 128;
        int skip_boundary_points = 10;
        std::vector<double> intermediate_radiuses;
        int oversat_threshold = 254;
    };

    PerspectiveNormalization();
    explicit PerspectiveNormalization(const Params& params) : params_(params) {}
    explicit PerspectiveNormalization(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<NormalizedIris> run(const IRImage& image,
                               const NoiseMask& noise_mask,
                               const GeometryPolygons& extrapolated_contours,
                               const EyeOrientation& eye_orientation) const;

private:
    Params params_;

    /// Generate src/dst correspondence grids.
    /// Returns (src_points, dst_points), each [num_rings][num_pts_per_ring].
    std::pair<std::vector<std::vector<cv::Point2f>>,
              std::vector<std::vector<cv::Point2f>>>
    generate_correspondences(const Contour& pupil_points,
                             const Contour& iris_points) const;
};

}  // namespace iris
