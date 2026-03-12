#pragma once

#include <string_view>
#include <unordered_map>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Extract contours from binary segmentation masks using OpenCV.
///
/// For each class (eyeball, iris, pupil), finds contours with
/// cv::findContours(RETR_TREE, CHAIN_APPROX_SIMPLE), filters to
/// parent contours only, and selects the largest by area.
class ContourExtractor : public Algorithm<ContourExtractor> {
public:
    static constexpr std::string_view kName = "ContourExtractor";

    struct Params {
        double relative_area_threshold = 0.03;
        double absolute_area_threshold = 0.0;
    };

    ContourExtractor() = default;
    explicit ContourExtractor(const Params& params) : params_(params) {}
    explicit ContourExtractor(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<GeometryPolygons> run(const GeometryMask& geometry_mask) const;

private:
    Params params_;
};

}  // namespace iris
