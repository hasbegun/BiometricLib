#pragma once

#include <string_view>
#include <unordered_map>
#include <utility>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Binarize a multi-label segmentation map into geometry and noise masks.
///
/// Applies per-class thresholds to the segmentation predictions to produce
/// binary masks for pupil, iris, eyeball (GeometryMask) and eyelashes (NoiseMask).
class SegmentationBinarizer : public Algorithm<SegmentationBinarizer> {
public:
    static constexpr std::string_view kName = "SegmentationBinarizer";

    struct Params {
        double eyeball_threshold = 0.5;
        double iris_threshold = 0.5;
        double pupil_threshold = 0.5;
        double eyelashes_threshold = 0.5;
    };

    SegmentationBinarizer() = default;
    explicit SegmentationBinarizer(const Params& params) : params_(params) {}
    explicit SegmentationBinarizer(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<std::pair<GeometryMask, NoiseMask>> run(const SegmentationMap& segmap) const;

private:
    Params params_;
};

}  // namespace iris
