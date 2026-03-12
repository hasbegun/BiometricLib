#pragma once

#include <string_view>
#include <unordered_map>
#include <vector>

#include <opencv2/core.hpp>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Iris normalization using linear (Daugman rubber-sheet) transformation.
class LinearNormalization : public Algorithm<LinearNormalization> {
public:
    static constexpr std::string_view kName = "LinearNormalization";

    struct Params {
        int res_in_r = 128;
        int oversat_threshold = 254;
    };

    LinearNormalization() = default;
    explicit LinearNormalization(const Params& params) : params_(params) {}
    explicit LinearNormalization(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<NormalizedIris> run(const IRImage& image,
                               const NoiseMask& noise_mask,
                               const GeometryPolygons& extrapolated_contours,
                               const EyeOrientation& eye_orientation) const;

private:
    Params params_;

    std::vector<std::vector<cv::Point>> generate_correspondences(
        const Contour& pupil_points, const Contour& iris_points) const;
};

}  // namespace iris
