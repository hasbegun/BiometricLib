# Round 2: Rethinking Eye Tracking Accuracy

## Test Results

Subject looked at tiles 1→8→64→57→1 (top-left → top-right → bottom-right → bottom-left → top-left). **None were matched.** The gaze output is uncorrelated with actual eye position.

---

## Root Cause Analysis

After deep analysis of the full pipeline (camera → pixel conversion → face detection → eye crop → pupil detection → iris ratio → gaze mapping → tile), **five fundamental problems** were identified:

### Bug 1: Crop Origin Mismatch (CRITICAL)

The iris ratio computation in `eyetrack_ffi.cpp` recomputes the crop origin to convert pupil coordinates from crop-space to frame-space. But it does **NOT** match the actual crop computation in `eye_extractor.cpp`:

**eye_extractor.cpp (actual crop):**
```cpp
float crop_x1_float = min_x - w * padding;
int x1 = std::max(0, static_cast<int>(std::floor(crop_x1_float)));  // floor + clamp to 0
```

**eyetrack_ffi.cpp (iris ratio recomputation):**
```cpp
float crop_x1 = min_x - w * padding;  // NO floor, NO clamp!
```

The `std::floor()` rounding and `std::max(0, ...)` clipping are **missing**. This means:
- The pupil frame-coordinate is wrong by up to 1+ pixels from floor
- If the eye is near the frame edge, `crop_x1` could be negative but actual crop starts at 0 — error of many pixels
- On a 56-pixel-wide eye, even a 2-pixel error = ~3.5% iris ratio error = ~12% gaze error after scaling

**This alone can explain the complete tracking failure.**

### Bug 2: Pupil Detection is Fundamentally Fragile

The current approach (adaptive threshold → largest contour → centroid) is unreliable at webcam resolution:

- **Eye crop is ~56×33 pixels.** Pupil is ~10-15px across.
- **Adaptive threshold block_size=11** is 20% of the image width — essentially a semi-global threshold. It picks up eyelashes, iris boundary, specular reflections, not just the pupil.
- **"Largest contour"** is often the eyelid margin or iris boundary, not the pupil.
- When looking at different tiles on the SAME screen, pupil shift is ~2-3 pixels. This is within the noise floor of contour detection.

**The proven alternative**: Instead of contour detection, use **weighted centroid of darkest region**. The pupil is the darkest part of the eye. Gaussian blur + inverted-intensity centroid naturally finds the pupil center without thresholds, contours, or circularity checks.

### Bug 3: No Per-User Calibration

The C++ FFI path uses hardcoded scaling ranges:
```cpp
constexpr float kIrisHMin = 0.35F;  // assumed minimum iris ratio
constexpr float kIrisHMax = 0.65F;  // assumed maximum iris ratio
```

These are **meaningless** without knowing the actual user's iris movement range. Different users, different distances, different screen sizes all produce different ranges. The Dart-side `GazeCalibrationData` exists but is **completely bypassed** in the FFI path:

```dart
if (_useFFI) {
    gazeX = result.gazeX;  // Uses hardcoded C++ ranges
    gazeY = result.gazeY;  // Calibration ignored!
}
```

### Bug 4: 8×8 Grid Resolution is Unrealistic

With a consumer webcam at ~720p:
- Face is ~200px wide in frame
- Each eye is ~56px wide in crop
- Pupil is ~12px across
- Looking at different parts of a 13" screen at 50cm produces ~2-3px pupil shift

To distinguish 8 columns requires ~0.4px precision per column. **This exceeds what blob detection on a consumer webcam can deliver.** Commercial eye trackers achieve ~1° accuracy using IR cameras + corneal reflection, which maps to roughly **4×4 grid** on a laptop screen.

### Bug 5: EMA Smoothing Creates Center Bias

With alpha=0.5, the EMA smoother averages current + previous frames equally. When pupil detection is noisy (which it is, per Bug 2), the smoothed value converges toward the mean of the noise distribution — which is ~0.5 (center). This means the gaze output gravitates toward center tiles regardless of where the user looks.

---

## Fix Plan

### Fix 1: Exact Crop Origin Computation (CRITICAL, immediate)

**File:** `src/ffi/eyetrack_ffi.cpp`

Replicate the **exact** same computation as `eye_extractor.cpp`:

```cpp
std::pair<float, float> compute_iris_ratio(
    const eyetrack::PupilInfo& pupil,
    const std::array<eyetrack::Point2D, 6>& ear,
    float padding,
    int frame_cols, int frame_rows) {  // NEW: pass frame dimensions for clipping

    // Recompute crop origin EXACTLY matching eye_extractor.cpp crop_eye()
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

    // MUST match eye_extractor.cpp: floor + clamp to frame bounds
    int crop_x1 = std::max(0, static_cast<int>(std::floor(min_x - w * padding)));
    int crop_y1 = std::max(0, static_cast<int>(std::floor(min_y - h * padding)));

    // Convert pupil from crop coords to frame coords
    float pupil_frame_x = pupil.center.x + static_cast<float>(crop_x1);
    float pupil_frame_y = pupil.center.y + static_cast<float>(crop_y1);

    // ... rest unchanged
}
```

### Fix 2: Robust Pupil Detection — Dark Region Centroid

**File:** `src/nodes/pupil_detector.cpp`

Replace adaptive threshold + contour approach with weighted centroid of darkest region:

```cpp
Result<PupilInfo> PupilDetector::run_centroid(const cv::Mat& gray) const {
    // 1. Gaussian blur to smooth out eyelashes and noise
    cv::Mat blurred;
    cv::GaussianBlur(gray, blurred, cv::Size(7, 7), 0);

    // 2. Invert: dark pixels (pupil) become bright (high weight)
    cv::Mat inverted;
    cv::bitwise_not(blurred, inverted);

    // 3. Threshold to focus on darkest 30% of pixels
    double minVal, maxVal;
    cv::minMaxLoc(blurred, &minVal, &maxVal);
    double threshold = minVal + (maxVal - minVal) * 0.3;
    cv::Mat mask;
    cv::threshold(blurred, mask, threshold, 255, cv::THRESH_BINARY_INV);

    // 4. Weighted centroid using inverted intensity as weights
    cv::Mat weighted;
    inverted.convertTo(weighted, CV_32F);
    cv::Mat masked_weight;
    mask.convertTo(masked_weight, CV_32F, 1.0/255.0);
    weighted = weighted.mul(masked_weight);

    cv::Moments m = cv::moments(weighted, false);
    if (m.m00 < 1e-6) {
        return make_error(ErrorCode::PupilNotDetected, "No dark region found");
    }

    float cx = static_cast<float>(m.m10 / m.m00);
    float cy = static_cast<float>(m.m01 / m.m00);

    // 5. Position validation: must be in center 70% of crop
    if (cx < gray.cols * 0.15F || cx > gray.cols * 0.85F ||
        cy < gray.rows * 0.15F || cy > gray.rows * 0.85F) {
        return make_error(ErrorCode::PupilNotDetected, "Dark centroid at edge");
    }

    // 6. Estimate radius from darkest region size
    float radius = static_cast<float>(std::sqrt(cv::countNonZero(mask) / CV_PI));

    PupilInfo result;
    result.center = {cx, cy};
    result.radius = radius;
    result.confidence = 0.8F;  // High base confidence — method is stable
    return result;
}
```

**Why this is better:**
- No contour detection = no "wrong contour" problem
- Gaussian blur filters eyelash noise
- Weighted centroid is sub-pixel precise
- The darkest region IS the pupil — physics-based, not heuristic
- Stable across lighting conditions (relative darkness, not absolute threshold)

### Fix 3: Apply Dart Calibration to FFI Path

**File:** `client/lib/presentation/screens/tracking_screen.dart`

Stop bypassing calibration for FFI:

```dart
// In _detectFromRawFrame:
double gazeX;
double gazeY;
if (_calibrationData != null) {
    // Apply calibration to raw iris ratios (works for both FFI and MethodChannel)
    gazeX = _calibrationData!.mapH(result.irisRatioH);
    gazeY = _calibrationData!.mapV(result.irisRatioV);
} else {
    gazeX = result.gazeX;
    gazeY = result.gazeY;
}
```

And remove the C++ side scaling + inversion (let Dart calibration handle it):

**File:** `src/ffi/eyetrack_ffi.cpp`

```cpp
// Return RAW iris ratios as gaze_x/gaze_y (no scaling, no inversion)
// Let Dart-side calibration handle the mapping
result.gaze_x = iris_h;
result.gaze_y = iris_v;
```

### Fix 4: Mandatory Calibration Flow

**File:** `client/lib/presentation/screens/tracking_screen.dart`

If no calibration data exists, force calibration before tracking:

```dart
if (_calibrationData == null) {
    // Show calibration prompt / auto-start calibration
    _showCalibrationRequired();
    return;
}
```

The existing `CalibrationScreen` + `GazeCalibrationData.fromSamples()` already handles collecting and computing per-user ranges. It just needs to be required, not optional.

### Fix 5: Reduce Default Grid to 4×4

**File:** `client/lib/presentation/screens/tracking_screen.dart`

Make grid size configurable, default to 4×4 as a realistic target:

```dart
static const int kGridSize = 4;  // Was 8 — unrealistic for webcam

int get gazeTile {
    final col = (_state.gazeX * kGridSize).floor().clamp(0, kGridSize - 1);
    final row = (_state.gazeY * kGridSize).floor().clamp(0, kGridSize - 1);
    return row * kGridSize + col + 1;
}
```

Once 4×4 works reliably, the grid size can be increased. 8×8 may eventually work with IR camera + calibration, but not with current hardware.

### Fix 6: Enhanced Debug Logging

**File:** `src/ffi/eyetrack_ffi.cpp`

Log EVERY frame (not every 30th) during debugging, with enough info to diagnose:

```cpp
fprintf(stderr,
    "[eyetrack] pupilL(%.1f,%.1f) pupilR(%.1f,%.1f) "
    "irisH=%.3f irisV=%.3f gazeX=%.3f gazeY=%.3f "
    "conf=%.2f ear=%.3f cropL(%d,%d) cropR(%d,%d)\n",
    left_cx, left_cy, right_cx, right_cy,
    iris_h, iris_v, gaze_x, gaze_y,
    confidence, ear_avg,
    left_crop_x1, left_crop_y1, right_crop_x1, right_crop_y1);
```

This allows us to see:
- Are pupil positions changing when the subject looks at different tiles?
- Are iris ratios differentiating left/right/up/down?
- Is the crop origin correct?

---

## Implementation Order

| Priority | Fix | Impact | Risk |
|----------|-----|--------|------|
| 1 | Fix 1: Crop origin mismatch | HIGH — current iris ratios are mathematically wrong | LOW — exact same logic as eye_extractor |
| 2 | Fix 2: Dark region centroid | HIGH — eliminates contour detection noise | MEDIUM — new algorithm, needs testing |
| 3 | Fix 6: Debug logging | HIGH — visibility into what's actually happening | NONE — logging only |
| 4 | Fix 3: Apply Dart calibration | HIGH — removes hardcoded ranges | LOW — uses existing code |
| 5 | Fix 4: Mandatory calibration | MEDIUM — user must calibrate | LOW — existing screen exists |
| 6 | Fix 5: Reduce grid to 4×4 | MEDIUM — achievable target | LOW — UI only |

---

## Files to Modify

| File | Changes |
|------|---------|
| `src/ffi/eyetrack_ffi.cpp` | Fix crop origin (floor+clamp); return raw iris ratios; enhanced logging |
| `src/nodes/pupil_detector.cpp` | Replace adaptive threshold+contour with dark region centroid |
| `client/lib/presentation/screens/tracking_screen.dart` | Apply calibration to FFI; reduce grid; require calibration |

---

## Verification

1. **Build dylib:** `bash client/scripts/build_eyetrack.sh`
2. **Run Flutter tests:** `cd client && fvm flutter test` (167 tests must pass)
3. **Build app:** `cd client && fvm flutter build macos`
4. **Run with debug logging:** Watch stderr output
   - Pupil positions should shift 2-5px when looking left vs right
   - Iris ratio H should vary (e.g., 0.40 vs 0.60)
   - After calibration, gaze_x should map correctly to tile columns
5. **Calibration test:** Complete 9-point calibration, then test grid tracking
6. **Target:** Reliable 4×4 grid tracking after calibration

---

## Honest Assessment

**8×8 grid (64 tiles) is extremely ambitious** for a consumer webcam with blob-based pupil detection. Commercial eye trackers (Tobii, etc.) use:
- Infrared illumination for consistent pupil contrast
- 60-120Hz dedicated cameras
- Corneal reflection (Purkinje images) for sub-pixel precision
- Achieve ~1° accuracy → roughly 4×4 grid on a laptop

**Realistic targets with current hardware:**
- 3×3 grid: Very achievable with calibration + dark centroid
- 4×4 grid: Achievable with good lighting + calibration
- 8×8 grid: Would require IR camera or significantly better hardware

The fixes above should get 4×4 working reliably. If 8×8 is truly required, the hardware approach needs to change (IR illumination, higher-res camera, or a commercial eye tracker SDK).
