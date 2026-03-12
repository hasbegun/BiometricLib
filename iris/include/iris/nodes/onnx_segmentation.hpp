#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

#ifdef IRIS_HAS_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif

namespace iris {

/// ONNX-based multilabel semantic segmentation for IR iris images.
///
/// Pipeline: resize -> [denoise] -> normalize -> ImageNet norm -> NCHW
///           -> ORT inference -> HWC -> resize back.
/// Only functional when built with IRIS_HAS_ONNXRUNTIME.
class OnnxSegmentation : public Algorithm<OnnxSegmentation> {
public:
    static constexpr std::string_view kName = "OnnxSegmentation";

    struct Params {
        std::string model_path;
        int input_width = 640;
        int input_height = 480;
        int input_num_channels = 3;
        bool denoise = false;
        int intra_op_num_threads = 0;  ///< 0 = ONNX default (all cores)
    };

    OnnxSegmentation() = delete;
    explicit OnnxSegmentation(const Params& params);
    explicit OnnxSegmentation(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<SegmentationMap> run(const IRImage& image) const;

private:
    Params params_;
#ifdef IRIS_HAS_ONNXRUNTIME
    /// ORT state wrapped in shared_ptr for copyability (std::any requires it).
    struct OrtState {
        Ort::SessionOptions options;
        Ort::Session session{nullptr};
        std::string input_name;
        std::string output_name;
    };
    std::shared_ptr<OrtState> ort_;

    void init_session();
#endif
};

}  // namespace iris
