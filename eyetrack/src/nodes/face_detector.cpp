#include <eyetrack/nodes/face_detector.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>

#include <opencv2/imgproc.hpp>

#include <eyetrack/core/node_registry.hpp>

#ifdef EYETRACK_HAS_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif

#ifdef EYETRACK_HAS_DLIB
#include <dlib/image_processing.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/opencv.h>
#endif

namespace eyetrack {

// ---- Pimpl structs ----

#ifdef EYETRACK_HAS_ONNXRUNTIME
struct FaceDetectorOnnx {
    Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "FaceDetector"};
    std::unique_ptr<Ort::Session> session;
    Ort::AllocatorWithDefaultOptions allocator;
};
#endif

#ifdef EYETRACK_HAS_DLIB
struct FaceDetectorDlib {
    dlib::frontal_face_detector detector = dlib::get_frontal_face_detector();
    dlib::shape_predictor shape_pred;
    bool loaded = false;
};
#endif

// ---- Static members ----

const std::vector<std::string> FaceDetector::kDependencies = {"eyetrack.Preprocessor"};

// ---- Constructors ----

FaceDetector::FaceDetector() = default;

FaceDetector::FaceDetector(const Config& config) : config_(config) {}

FaceDetector::FaceDetector(const NodeParams& params) {
    if (auto it = params.find("model_path"); it != params.end())
        config_.model_path = it->second;
    if (auto it = params.find("dlib_shape_predictor_path"); it != params.end())
        config_.dlib_shape_predictor_path = it->second;
    if (auto it = params.find("confidence_threshold"); it != params.end())
        config_.confidence_threshold = std::stof(it->second);
    if (auto it = params.find("backend"); it != params.end()) {
        if (it->second == "dlib")
            config_.backend = FaceDetectorBackend::Dlib;
        else
            config_.backend = FaceDetectorBackend::MediaPipe;
    }
}

// ---- Shared helpers ----

Result<cv::Mat> FaceDetector::preprocess(const cv::Mat& input) {
    if (input.empty()) {
        return make_error(ErrorCode::InvalidInput, "Empty input frame");
    }

    cv::Mat resized;
    cv::resize(input, resized, cv::Size(kModelInputSize, kModelInputSize));

    cv::Mat rgb;
    cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);

    cv::Mat normalized;
    rgb.convertTo(normalized, CV_32FC3, 1.0 / 255.0);

    return normalized;
}

std::array<Point2D, 6> FaceDetector::dlib_ear_subset(
    const std::vector<Point2D>& landmarks_68,
    const std::array<int, 6>& indices) {
    std::array<Point2D, 6> ear{};
    for (size_t i = 0; i < 6; ++i) {
        ear[i] = landmarks_68[static_cast<size_t>(indices[i])];
    }
    return ear;
}

// ---- Bounding box from landmarks helper ----

namespace {

FaceROI make_face_roi(std::vector<Point2D> landmarks, float confidence,
                      float frame_w, float frame_h) {
    float min_x = std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float max_y = std::numeric_limits<float>::lowest();

    for (auto& lm : landmarks) {
        lm.x = std::clamp(lm.x, 0.0F, frame_w);
        lm.y = std::clamp(lm.y, 0.0F, frame_h);
        min_x = std::min(min_x, lm.x);
        min_y = std::min(min_y, lm.y);
        max_x = std::max(max_x, lm.x);
        max_y = std::max(max_y, lm.y);
    }

    FaceROI roi;
    roi.bounding_box = cv::Rect2f(min_x, min_y, max_x - min_x, max_y - min_y);
    roi.landmarks = std::move(landmarks);
    roi.confidence = confidence;
    return roi;
}

}  // namespace

// ===========================================================================
// MediaPipe ONNX backend
// ===========================================================================

#ifdef EYETRACK_HAS_ONNXRUNTIME

Result<void> FaceDetector::load_onnx_model() const {
    if (onnx_ && onnx_->session) {
        return {};
    }

    if (!std::filesystem::exists(config_.model_path)) {
        return make_error(ErrorCode::ModelLoadFailed,
                          "Model file not found", config_.model_path);
    }

    try {
        if (!onnx_) {
            onnx_ = std::make_shared<FaceDetectorOnnx>();
        }
        Ort::SessionOptions opts;
        opts.SetIntraOpNumThreads(1);
        opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        onnx_->session = std::make_unique<Ort::Session>(
            onnx_->env, config_.model_path.c_str(), opts);
    } catch (const Ort::Exception& e) {
        return make_error(ErrorCode::ModelLoadFailed,
                          "ONNX Runtime session creation failed", e.what());
    }

    return {};
}

Result<FaceROI> FaceDetector::run_mediapipe(const cv::Mat& input) const {
    auto load_result = load_onnx_model();
    if (!load_result) return std::unexpected(load_result.error());

    auto preprocessed = preprocess(input);
    if (!preprocessed) return std::unexpected(preprocessed.error());

    const cv::Mat& tensor_mat = preprocessed.value();

    std::array<int64_t, 4> input_shape = {1, kModelInputSize, kModelInputSize, 3};
    const auto input_tensor_size =
        static_cast<size_t>(kModelInputSize * kModelInputSize * 3);

    Ort::MemoryInfo mem = Ort::MemoryInfo::CreateCpu(
        OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);

    auto input_tensor = Ort::Value::CreateTensor<float>(
        mem, const_cast<float*>(tensor_mat.ptr<float>()),
        input_tensor_size, input_shape.data(), input_shape.size());

    auto in_name = onnx_->session->GetInputNameAllocated(0, onnx_->allocator);
    auto out_lm = onnx_->session->GetOutputNameAllocated(0, onnx_->allocator);
    auto out_conf = onnx_->session->GetOutputNameAllocated(1, onnx_->allocator);

    const char* in_names[] = {in_name.get()};
    const char* out_names[] = {out_lm.get(), out_conf.get()};

    std::vector<Ort::Value> outputs;
    try {
        outputs = onnx_->session->Run(
            Ort::RunOptions{nullptr}, in_names, &input_tensor, 1, out_names, 2);
    } catch (const Ort::Exception& e) {
        return make_error(ErrorCode::ModelLoadFailed,
                          "ONNX Runtime inference failed", e.what());
    }

    float confidence = outputs[1].GetTensorData<float>()[0];
    if (confidence < config_.confidence_threshold) {
        return make_error(ErrorCode::FaceNotDetected,
                          "Face confidence below threshold",
                          "confidence=" + std::to_string(confidence) +
                          " threshold=" + std::to_string(config_.confidence_threshold));
    }

    const float* lm_data = outputs[0].GetTensorData<float>();
    const auto orig_w = static_cast<float>(input.cols);
    const auto orig_h = static_cast<float>(input.rows);

    std::vector<Point2D> landmarks;
    landmarks.reserve(kNumLandmarksMediaPipe);
    for (int i = 0; i < kNumLandmarksMediaPipe; ++i) {
        float x = lm_data[i * kLandmarkDims] * orig_w;
        float y = lm_data[i * kLandmarkDims + 1] * orig_h;
        landmarks.push_back({x, y});
    }

    return make_face_roi(std::move(landmarks), confidence, orig_w, orig_h);
}

#endif  // EYETRACK_HAS_ONNXRUNTIME

// ===========================================================================
// dlib HOG + shape predictor backend
// ===========================================================================

#ifdef EYETRACK_HAS_DLIB

Result<void> FaceDetector::load_dlib_model() const {
    if (dlib_ && dlib_->loaded) {
        return {};
    }

    if (!std::filesystem::exists(config_.dlib_shape_predictor_path)) {
        return make_error(ErrorCode::ModelLoadFailed,
                          "dlib shape predictor file not found",
                          config_.dlib_shape_predictor_path);
    }

    try {
        if (!dlib_) {
            dlib_ = std::make_shared<FaceDetectorDlib>();
        }
        dlib::deserialize(config_.dlib_shape_predictor_path) >> dlib_->shape_pred;
        dlib_->loaded = true;
    } catch (const dlib::serialization_error& e) {
        return make_error(ErrorCode::ModelLoadFailed,
                          "Failed to load dlib shape predictor", e.what());
    }

    return {};
}

Result<FaceROI> FaceDetector::run_dlib(const cv::Mat& input) const {
    auto load_result = load_dlib_model();
    if (!load_result) return std::unexpected(load_result.error());

    // Convert to dlib image (wraps cv::Mat, no copy)
    cv::Mat bgr;
    if (input.channels() == 1) {
        cv::cvtColor(input, bgr, cv::COLOR_GRAY2BGR);
    } else {
        bgr = input;
    }

    dlib::cv_image<dlib::bgr_pixel> dlib_img(bgr);

    // Detect faces with HOG detector
    std::vector<dlib::rectangle> faces = dlib_->detector(dlib_img);
    if (faces.empty()) {
        return make_error(ErrorCode::FaceNotDetected,
                          "dlib HOG detector found no faces");
    }

    // Use the largest face (by area)
    auto& face_rect = *std::max_element(faces.begin(), faces.end(),
        [](const dlib::rectangle& a, const dlib::rectangle& b) {
            return a.area() < b.area();
        });

    // Get 68 landmarks
    dlib::full_object_detection shape = dlib_->shape_pred(dlib_img, face_rect);

    const auto orig_w = static_cast<float>(input.cols);
    const auto orig_h = static_cast<float>(input.rows);

    std::vector<Point2D> landmarks;
    landmarks.reserve(kNumLandmarksDlib);
    for (unsigned long i = 0; i < shape.num_parts(); ++i) {
        const auto& pt = shape.part(i);
        landmarks.push_back({static_cast<float>(pt.x()), static_cast<float>(pt.y())});
    }

    // dlib doesn't give a confidence score per-se; use 1.0 if detected.
    return make_face_roi(std::move(landmarks), 1.0F, orig_w, orig_h);
}

#endif  // EYETRACK_HAS_DLIB

// ===========================================================================
// Dispatch: run()
// ===========================================================================

Result<FaceROI> FaceDetector::run(const cv::Mat& input) const {
    if (input.empty()) {
        return make_error(ErrorCode::InvalidInput, "Empty input frame");
    }

    if (config_.backend == FaceDetectorBackend::Dlib) {
#ifdef EYETRACK_HAS_DLIB
        return run_dlib(input);
#else
        return make_error(ErrorCode::FaceNotDetected,
                          "dlib backend not available",
                          "Build with EYETRACK_HAS_DLIB to enable dlib face detection");
#endif
    }

    // Default: MediaPipe
#ifdef EYETRACK_HAS_ONNXRUNTIME
    return run_mediapipe(input);
#else
    return make_error(ErrorCode::FaceNotDetected,
                      "ONNX Runtime not available (stub fallback)",
                      "Build with EYETRACK_HAS_ONNXRUNTIME to enable face detection");
#endif
}

// ===========================================================================
// Pipeline integration
// ===========================================================================

Result<void> FaceDetector::execute(
    std::unordered_map<std::string, std::any>& context) const {
    cv::Mat input;
    if (auto it = context.find("preprocessed_color"); it != context.end()) {
        input = std::any_cast<const cv::Mat&>(it->second);
    } else if (auto it = context.find("frame"); it != context.end()) {
        input = std::any_cast<const cv::Mat&>(it->second);
    } else {
        return make_error(ErrorCode::InvalidInput,
                          "Missing 'preprocessed_color' or 'frame' in pipeline context");
    }

    auto result = run(input);
    if (!result) return std::unexpected(result.error());

    context["face_roi"] = std::move(result.value());
    return {};
}

const std::vector<std::string>& FaceDetector::dependencies() const noexcept {
    return kDependencies;
}

EYETRACK_REGISTER_NODE("eyetrack.FaceDetector", FaceDetector);

}  // namespace eyetrack
