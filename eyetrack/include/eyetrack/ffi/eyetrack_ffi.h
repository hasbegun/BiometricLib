#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void* EyetrackHandle;

typedef struct {
    int detected;
    float gaze_x;
    float gaze_y;
    float confidence;
    int blink_type;       // 0=none, 1=single, 2=double, 3=long
    float ear_left;
    float ear_right;
    float iris_ratio_h;   // pupil horizontal position in eye (0=left, 1=right)
    float iris_ratio_v;   // pupil vertical position in eye (0=top, 1=bottom)
    float left_pupil_x;   // normalized pupil center (0-1 within eye crop)
    float left_pupil_y;
    float right_pupil_x;
    float right_pupil_y;
    float face_x;         // face bounding box (pixels)
    float face_y;
    float face_w;
    float face_h;
} EyetrackResultC;

/// Create an eye tracker instance. Loads the dlib shape predictor model.
/// model_dir: path to directory containing shape_predictor_68_face_landmarks.dat
/// Returns NULL on failure.
EyetrackHandle eyetrack_create(const char* model_dir);

/// Destroy an eye tracker instance and free resources.
void eyetrack_destroy(EyetrackHandle handle);

/// Process a single video frame and return eye tracking results.
/// pixel_data: raw pixel buffer
/// width, height: frame dimensions
/// bytes_per_row: stride
/// pixel_format: 0=BGRA, 1=ARGB
EyetrackResultC eyetrack_process_frame(
    EyetrackHandle handle,
    const unsigned char* pixel_data,
    int width, int height, int bytes_per_row,
    int pixel_format);

/// Set EMA smoothing alpha (0.0 = no update, 1.0 = no smoothing).
void eyetrack_set_smoothing(EyetrackHandle handle, float alpha);

/// Reset smoother and blink detector state.
void eyetrack_reset(EyetrackHandle handle);

#ifdef __cplusplus
}
#endif
