#include <iris/nodes/onnx_segmentation.hpp>

#ifdef IRIS_HAS_ONNXRUNTIME

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <opencv2/imgproc.hpp>

#include <iris/core/node_registry.hpp>

namespace iris {

namespace {

/// Singleton ONNX Runtime environment.
Ort::Env& ort_env() {
    static Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "iris"};
    return env;
}

/// ImageNet RGB normalization constants.
constexpr std::array<double, 3> kImageNetMean = {0.485, 0.456, 0.406};
constexpr std::array<double, 3> kImageNetStd = {0.229, 0.224, 0.225};

/// Selective bilateral filter denoising.
/// Only applies filtering to pixels with intensity >= threshold.
cv::Mat image_denoise(const cv::Mat& img, int d, double sigma_color,
                      double sigma_space, int intensity_threshold) {
    cv::Mat result = img.clone();
    cv::Mat filtered;
    cv::bilateralFilter(img, filtered, d, sigma_color, sigma_space);

    for (int y = 0; y < img.rows; ++y) {
        const auto* src_row = img.ptr<uint8_t>(y);
        const auto* flt_row = filtered.ptr<uint8_t>(y);
        auto* dst_row = result.ptr<uint8_t>(y);
        for (int x = 0; x < img.cols; ++x) {
            if (src_row[x] >= static_cast<uint8_t>(intensity_threshold)) {
                dst_row[x] = flt_row[x];
            }
        }
    }
    return result;
}

}  // namespace

void OnnxSegmentation::init_session() {
    ort_ = std::make_shared<OrtState>();

    ort_->options.SetIntraOpNumThreads(params_.intra_op_num_threads);
    ort_->options.SetGraphOptimizationLevel(
        GraphOptimizationLevel::ORT_ENABLE_ALL);

    ort_->session = Ort::Session(ort_env(), params_.model_path.c_str(),
                                 ort_->options);

    Ort::AllocatorWithDefaultOptions allocator;
    ort_->input_name =
        ort_->session.GetInputNameAllocated(0, allocator).get();
    ort_->output_name =
        ort_->session.GetOutputNameAllocated(0, allocator).get();
}

OnnxSegmentation::OnnxSegmentation(const Params& params) : params_(params) {
    init_session();
}

OnnxSegmentation::OnnxSegmentation(
    const std::unordered_map<std::string, std::string>& node_params) {
    auto it = node_params.find("model_path");
    if (it != node_params.end()) params_.model_path = it->second;

    it = node_params.find("input_width");
    if (it != node_params.end()) params_.input_width = std::stoi(it->second);

    it = node_params.find("input_height");
    if (it != node_params.end()) params_.input_height = std::stoi(it->second);

    it = node_params.find("input_num_channels");
    if (it != node_params.end())
        params_.input_num_channels = std::stoi(it->second);

    it = node_params.find("denoise");
    if (it != node_params.end()) {
        params_.denoise = (it->second == "true" || it->second == "1");
    }

    it = node_params.find("intra_op_num_threads");
    if (it != node_params.end()) {
        params_.intra_op_num_threads = std::stoi(it->second);
    }

    init_session();
}

Result<SegmentationMap> OnnxSegmentation::run(const IRImage& image) const {
    if (image.data.empty()) {
        return make_error(ErrorCode::SegmentationFailed, "Empty input image");
    }

    const int orig_width = image.data.cols;
    const int orig_height = image.data.rows;

    // 1. Resize to model input resolution.
    //    Match Python: cv2.resize(image.astype(float), input_resolution)
    //    then astype(np.uint8) — resize on float64, truncate back to uint8.
    cv::Mat float_img;
    image.data.convertTo(float_img, CV_64FC1);
    cv::Mat resized_float;
    cv::resize(float_img, resized_float,
               cv::Size(params_.input_width, params_.input_height));
    // Truncate to uint8 (matching numpy astype(np.uint8), NOT rounding)
    cv::Mat resized(resized_float.size(), CV_8UC1);
    for (int ry = 0; ry < resized_float.rows; ++ry) {
        const auto* src_row = resized_float.ptr<double>(ry);
        auto* dst_row = resized.ptr<uint8_t>(ry);
        for (int rx = 0; rx < resized_float.cols; ++rx) {
            const double v = std::clamp(src_row[rx], 0.0, 255.0);
            dst_row[rx] = static_cast<uint8_t>(v);
        }
    }

    // 2. Optional bilateral-filter denoising
    if (params_.denoise) {
        resized = image_denoise(resized, 5, 75.0, 10.0, 75);
    }

    // 3. Normalize to [0, 1] (replicates torchvision.ToTensor)
    cv::Mat normalized;
    resized.convertTo(normalized, CV_64FC1, 1.0 / 255.0);

    // 4. Tile to input_num_channels (grayscale -> "RGB")
    const int nc = params_.input_num_channels;
    std::vector<cv::Mat> channels(static_cast<size_t>(nc));
    for (int c = 0; c < nc; ++c) {
        channels[static_cast<size_t>(c)] = normalized.clone();
    }

    // 5. ImageNet normalization per channel
    for (int c = 0; c < nc; ++c) {
        double mean = 0.5;
        double std_val = 0.5;
        if (nc == 3) {
            mean = kImageNetMean[static_cast<size_t>(c)];
            std_val = kImageNetStd[static_cast<size_t>(c)];
        }
        channels[static_cast<size_t>(c)] =
            (channels[static_cast<size_t>(c)] - mean) / std_val;
    }

    // 6. Convert to NCHW float32 tensor [1, C, H, W]
    const auto h = static_cast<int64_t>(params_.input_height);
    const auto w = static_cast<int64_t>(params_.input_width);
    const auto c_dim = static_cast<int64_t>(nc);
    const auto plane_size = static_cast<size_t>(h * w);

    std::vector<float> input_data(static_cast<size_t>(c_dim) * plane_size);

    for (int c = 0; c < nc; ++c) {
        cv::Mat float_chan;
        channels[static_cast<size_t>(c)].convertTo(float_chan, CV_32FC1);

        const float* src = float_chan.ptr<float>();
        float* dst = input_data.data() + static_cast<size_t>(c) * plane_size;
        std::copy(src, src + plane_size, dst);
    }

    // Create ORT input tensor
    std::array<int64_t, 4> input_shape = {1, c_dim, h, w};
    auto mem_info =
        Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        mem_info, input_data.data(), input_data.size(), input_shape.data(),
        input_shape.size());

    // 7. Run inference
    const char* in_names[] = {ort_->input_name.c_str()};
    const char* out_names[] = {ort_->output_name.c_str()};

    std::vector<Ort::Value> outputs;
    try {
        outputs = ort_->session.Run(Ort::RunOptions{nullptr}, in_names,
                                    &input_tensor, 1, out_names, 1);
    } catch (const Ort::Exception& e) {
        return make_error(ErrorCode::SegmentationFailed,
                          std::string("ONNX inference failed: ") + e.what());
    }

    if (outputs.empty() || !outputs[0].IsTensor()) {
        return make_error(ErrorCode::SegmentationFailed,
                          "No valid output tensor");
    }

    // 8. Postprocess: squeeze batch, NCHW -> HWC, resize to original
    auto shape = outputs[0].GetTensorTypeAndShapeInfo().GetShape();
    const int num_classes = static_cast<int>(shape[1]);
    const int out_h = static_cast<int>(shape[2]);
    const int out_w = static_cast<int>(shape[3]);

    const float* out_data = outputs[0].GetTensorData<float>();

    cv::Mat segmap(out_h, out_w, CV_MAKETYPE(CV_32F, num_classes));

    for (int cls = 0; cls < num_classes; ++cls) {
        for (int y = 0; y < out_h; ++y) {
            float* row = segmap.ptr<float>(y);
            for (int x = 0; x < out_w; ++x) {
                const auto src_idx = static_cast<size_t>(
                    cls * out_h * out_w + y * out_w + x);
                row[x * num_classes + cls] = out_data[src_idx];
            }
        }
    }

    cv::Mat resized_segmap;
    cv::resize(segmap, resized_segmap, cv::Size(orig_width, orig_height), 0, 0,
               cv::INTER_NEAREST);

    return SegmentationMap{.predictions = std::move(resized_segmap)};
}

IRIS_REGISTER_NODE("iris.MultilabelSegmentation.create_from_hugging_face",
                    OnnxSegmentation);

}  // namespace iris

#else  // !IRIS_HAS_ONNXRUNTIME

namespace iris {

OnnxSegmentation::OnnxSegmentation(const Params& params) : params_(params) {}

OnnxSegmentation::OnnxSegmentation(
    const std::unordered_map<std::string, std::string>&) {}

Result<SegmentationMap> OnnxSegmentation::run(const IRImage&) const {
    return make_error(
        ErrorCode::SegmentationFailed,
        "ONNX Runtime not available — rebuild with IRIS_HAS_ONNXRUNTIME");
}

}  // namespace iris

#endif
