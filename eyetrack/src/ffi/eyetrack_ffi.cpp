#include <eyetrack/ffi/eyetrack_ffi.h>

#include <cmath>
#include <cstdio>
#include <string>

#include <opencv2/imgproc.hpp>

#include <eyetrack/core/types.hpp>
#include <eyetrack/nodes/blink_detector.hpp>
#include <eyetrack/nodes/eye_extractor.hpp>
#include <eyetrack/nodes/face_detector.hpp>
#include <eyetrack/nodes/gaze_smoother.hpp>
#include <eyetrack/nodes/pupil_detector.hpp>
#include <eyetrack/utils/geometry_utils.hpp>

namespace {

// -- Iris ratio range calibration --
// Landmark-relative iris ratio (pupil position between eye corners) typically
// varies ~0.35-0.65 across full left/right gaze range.
constexpr float kIrisHMin = 0.35F;
constexpr float kIrisHMax = 0.65F;
constexpr float kIrisVMin = 0.35F;
constexpr float kIrisVMax = 0.65F;

// Default eye extractor padding (must match EyeExtractor::Config::padding)
constexpr float kEyeCropPadding = 0.2F;

struct EyetrackContext {
    eyetrack::FaceDetector face_detector;
    eyetrack::EyeExtractor eye_extractor;
    eyetrack::PupilDetector pupil_detector;
    eyetrack::BlinkDetector blink_detector;
    eyetrack::GazeSmoother smoother;
};

/// Convert ARGB pixel buffer to BGR cv::Mat.
cv::Mat argb_to_bgr(const unsigned char* data, int width, int height, int bpr) {
    // ARGB8888: A R G B per pixel
    cv::Mat argb(height, width, CV_8UC4, const_cast<unsigned char*>(data), static_cast<size_t>(bpr));
    cv::Mat bgr;
    // ARGB -> RGBA first (swap A from front to back), then RGBA -> BGR
    // Or manually: ARGB channels are [A,R,G,B] = [0,1,2,3]
    // We need BGR = [B,G,R] = [3,2,1]
    cv::Mat channels[4];
    cv::split(argb, channels);
    // channels[0]=A, channels[1]=R, channels[2]=G, channels[3]=B
    cv::Mat bgr_channels[3] = {channels[3], channels[2], channels[1]};
    cv::merge(bgr_channels, 3, bgr);
    return bgr;
}

/// Convert BGRA pixel buffer to BGR cv::Mat.
cv::Mat bgra_to_bgr(const unsigned char* data, int width, int height, int bpr) {
    cv::Mat bgra(height, width, CV_8UC4, const_cast<unsigned char*>(data), static_cast<size_t>(bpr));
    cv::Mat bgr;
    cv::cvtColor(bgra, bgr, cv::COLOR_BGRA2BGR);
    return bgr;
}

/// Compute landmark-relative iris ratio using eye corner landmarks.
/// Pupil center is in crop coordinates; EAR landmarks are in frame coordinates.
/// We recompute the crop origin to convert, then measure pupil position
/// relative to eye corners (stable) instead of crop dimensions (arbitrary).
/// Returns (h_ratio, v_ratio) where 0.0 = left/top, 1.0 = right/bottom.
std::pair<float, float> compute_iris_ratio(
    const eyetrack::PupilInfo& pupil,
    const std::array<eyetrack::Point2D, 6>& ear,
    float padding) {
    // 1. Recompute crop bounding box from EAR landmarks (mirrors crop_eye logic)
    float min_x = ear[0].x, max_x = ear[0].x;
    float min_y = ear[0].y, max_y = ear[0].y;
    for (int i = 1; i < 6; ++i) {
        min_x = std::min(min_x, ear[static_cast<size_t>(i)].x);
        max_x = std::max(max_x, ear[static_cast<size_t>(i)].x);
        min_y = std::min(min_y, ear[static_cast<size_t>(i)].y);
        max_y = std::max(max_y, ear[static_cast<size_t>(i)].y);
    }
    float w = max_x - min_x;
    float h = max_y - min_y;
    float crop_x1 = min_x - w * padding;
    float crop_y1 = min_y - h * padding;

    // 2. Convert pupil center from crop coords to frame coords
    float pupil_frame_x = pupil.center.x + crop_x1;
    float pupil_frame_y = pupil.center.y + crop_y1;

    // 3. Eye corners: use min/max x of positions 0 and 3 (outer/inner corners)
    float corner_left_x  = std::min(ear[0].x, ear[3].x);
    float corner_right_x = std::max(ear[0].x, ear[3].x);
    float span_h = corner_right_x - corner_left_x;

    // Vertical: upper lid midpoint (p1,p2) to lower lid midpoint (p4,p5)
    float top_y = (ear[1].y + ear[2].y) / 2.0F;
    float bot_y = (ear[4].y + ear[5].y) / 2.0F;
    float span_v = bot_y - top_y;

    if (span_h < 1.0F || std::abs(span_v) < 1.0F) return {0.5F, 0.5F};

    // 4. Landmark-relative iris ratio
    float ratio_h = std::clamp((pupil_frame_x - corner_left_x) / span_h, 0.0F, 1.0F);
    float ratio_v = std::clamp((pupil_frame_y - top_y) / span_v, 0.0F, 1.0F);
    return {ratio_h, ratio_v};
}

EyetrackResultC make_empty_result() {
    EyetrackResultC r{};
    r.detected = 0;
    r.gaze_x = 0.5F;
    r.gaze_y = 0.5F;
    return r;
}

}  // namespace

extern "C" {

EyetrackHandle eyetrack_create(const char* model_dir) {
    try {
        auto* ctx = new EyetrackContext();

        // Configure face detector for dlib backend
        eyetrack::FaceDetector::Config fd_config;
        fd_config.backend = eyetrack::FaceDetectorBackend::Dlib;
        fd_config.dlib_shape_predictor_path =
            std::string(model_dir) + "/shape_predictor_68_face_landmarks.dat";
        ctx->face_detector = eyetrack::FaceDetector(fd_config);

        // Eye extractor with default padding
        ctx->eye_extractor = eyetrack::EyeExtractor();

        // Pupil detector using centroid method
        ctx->pupil_detector = eyetrack::PupilDetector();

        // Blink detector with default thresholds
        ctx->blink_detector = eyetrack::BlinkDetector();

        // Smoother with moderate alpha
        eyetrack::GazeSmoother::Config sm_config;
        sm_config.alpha = 0.5F;
        ctx->smoother = eyetrack::GazeSmoother(sm_config);

        // Force-load the dlib model now to catch errors early
        auto test_frame = cv::Mat::zeros(100, 100, CV_8UC3);
        // We just test that the object was created; model loads lazily on first run()
        (void)test_frame;

        return static_cast<EyetrackHandle>(ctx);
    } catch (...) {
        return nullptr;
    }
}

void eyetrack_destroy(EyetrackHandle handle) {
    if (handle) {
        delete static_cast<EyetrackContext*>(handle);
    }
}

EyetrackResultC eyetrack_process_frame(
    EyetrackHandle handle,
    const unsigned char* pixel_data,
    int width, int height, int bytes_per_row,
    int pixel_format) {

    if (!handle || !pixel_data || width <= 0 || height <= 0) {
        return make_empty_result();
    }

    auto* ctx = static_cast<EyetrackContext*>(handle);

    try {
        // 1. Convert pixel buffer to BGR cv::Mat
        cv::Mat bgr;
        if (pixel_format == 1) {
            bgr = argb_to_bgr(pixel_data, width, height, bytes_per_row);
        } else {
            bgr = bgra_to_bgr(pixel_data, width, height, bytes_per_row);
        }

        // 2. Face detection (dlib HOG + 68 landmarks)
        auto face_result = ctx->face_detector.run(bgr);
        if (!face_result) {
            return make_empty_result();
        }
        const auto& face = face_result.value();

        // 3. Eye extraction (crop eye regions using landmarks)
        auto eye_result = ctx->eye_extractor.run(face, bgr);
        if (!eye_result) {
            return make_empty_result();
        }
        const auto& eyes = eye_result.value();

        // 4. Pupil detection in both eyes
        auto left_pupil_result = ctx->pupil_detector.run(eyes.left_eye_crop);
        auto right_pupil_result = ctx->pupil_detector.run(eyes.right_eye_crop);

        bool has_left = left_pupil_result.has_value();
        bool has_right = right_pupil_result.has_value();

        if (!has_left && !has_right) {
            return make_empty_result();
        }

        // 5. Compute landmark-relative iris ratios
        float iris_h = 0.5F;
        float iris_v = 0.5F;
        float left_px = 0.5F, left_py = 0.5F;
        float right_px = 0.5F, right_py = 0.5F;
        float confidence = 0.0F;

        if (has_left) {
            auto [lh, lv] = compute_iris_ratio(
                left_pupil_result.value(), eyes.landmarks.left, kEyeCropPadding);
            left_px = lh;
            left_py = lv;
        }
        if (has_right) {
            auto [rh, rv] = compute_iris_ratio(
                right_pupil_result.value(), eyes.landmarks.right, kEyeCropPadding);
            right_px = rh;
            right_py = rv;
        }

        // Average iris ratios from both eyes
        if (has_left && has_right) {
            iris_h = (left_px + right_px) / 2.0F;
            iris_v = (left_py + right_py) / 2.0F;
            confidence = (left_pupil_result->confidence + right_pupil_result->confidence) / 2.0F;
        } else if (has_left) {
            iris_h = left_px;
            iris_v = left_py;
            confidence = left_pupil_result->confidence;
        } else {
            iris_h = right_px;
            iris_v = right_py;
            confidence = right_pupil_result->confidence;
        }

        // 6. Map landmark-relative iris ratio to gaze coordinates
        //
        // Rescale from typical range (~0.35-0.65) to full 0.0-1.0.
        float scaled_h = std::clamp(
            (iris_h - kIrisHMin) / (kIrisHMax - kIrisHMin), 0.0F, 1.0F);
        float scaled_v = std::clamp(
            (iris_v - kIrisVMin) / (kIrisVMax - kIrisVMin), 0.0F, 1.0F);

        // Camera is mirrored (isVideoMirrored=true):
        //   User looks LEFT → pupil moves RIGHT in mirrored image → iris_h HIGH
        //   Screen left = gaze_x 0.0 → invert horizontal
        // Vertical: direct mapping (looking down = iris_v high = gaze_y high)
        float gaze_x = 1.0F - scaled_h;
        float gaze_y = scaled_v;

        // 7. Smooth the gaze
        eyetrack::GazeResult raw_gaze;
        raw_gaze.gaze_point = {gaze_x, gaze_y};
        raw_gaze.confidence = confidence;
        raw_gaze.timestamp = std::chrono::steady_clock::now();

        auto smoothed = ctx->smoother.smooth(raw_gaze);

        // 8. Compute EAR and blink detection
        float ear_left = eyetrack::compute_ear(eyes.landmarks.left);
        float ear_right = eyetrack::compute_ear(eyes.landmarks.right);
        float ear_avg = (ear_left + ear_right) / 2.0F;

        auto blink = ctx->blink_detector.update(ear_avg, raw_gaze.timestamp);
        smoothed.blink = blink;

        // 9. Debug logging (every ~30 frames to avoid spam)
        static int frame_counter = 0;
        if (++frame_counter % 30 == 0) {
            fprintf(stderr,
                "[eyetrack] iris(%.3f,%.3f) scaled(%.3f,%.3f) "
                "gaze(%.3f,%.3f) smooth(%.3f,%.3f) "
                "conf=%.2f ear=%.3f\n",
                static_cast<double>(iris_h), static_cast<double>(iris_v),
                static_cast<double>(scaled_h), static_cast<double>(scaled_v),
                static_cast<double>(gaze_x), static_cast<double>(gaze_y),
                static_cast<double>(smoothed.gaze_point.x),
                static_cast<double>(smoothed.gaze_point.y),
                static_cast<double>(confidence), static_cast<double>(ear_avg));
        }

        // 10. Build result
        EyetrackResultC result{};
        result.detected = 1;
        result.gaze_x = smoothed.gaze_point.x;
        result.gaze_y = smoothed.gaze_point.y;
        result.confidence = smoothed.confidence;
        result.blink_type = static_cast<int>(smoothed.blink);
        result.ear_left = ear_left;
        result.ear_right = ear_right;
        result.iris_ratio_h = iris_h;
        result.iris_ratio_v = iris_v;
        result.left_pupil_x = left_px;
        result.left_pupil_y = left_py;
        result.right_pupil_x = right_px;
        result.right_pupil_y = right_py;
        result.face_x = face.bounding_box.x;
        result.face_y = face.bounding_box.y;
        result.face_w = face.bounding_box.width;
        result.face_h = face.bounding_box.height;

        return result;

    } catch (...) {
        return make_empty_result();
    }
}

void eyetrack_set_smoothing(EyetrackHandle handle, float alpha) {
    if (!handle) return;
    auto* ctx = static_cast<EyetrackContext*>(handle);
    eyetrack::GazeSmoother::Config config;
    config.alpha = alpha;
    ctx->smoother = eyetrack::GazeSmoother(config);
}

void eyetrack_reset(EyetrackHandle handle) {
    if (!handle) return;
    auto* ctx = static_cast<EyetrackContext*>(handle);
    ctx->smoother.reset();
    ctx->blink_detector.reset();
}

}  // extern "C"
