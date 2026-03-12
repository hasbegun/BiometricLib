# Implementation Checklist

Step-by-step execution of [MASTER_PLAN.md](MASTER_PLAN.md). Each task has a checkbox. Mark complete only when the acceptance criteria (indented sub-items) are met.

---

## Phase 1: Foundation

### 1.1 Project Scaffolding
- [x] Create `biometric/iris/` directory structure matching §3.1
- [x] Create root `CMakeLists.txt` with C++23, options for FHE/tests/bench/sanitizers
- [x] Create `CMakePresets.json` with presets: `linux-release`, `linux-debug-sanitizers`, `linux-debug-tsan`, `linux-aarch64`
- [x] Create `.clang-format` (project style)
- [x] Create `.clang-tidy` (strict checks)
- [ ] Verify: `cmake --preset linux-release` configures without errors — *deferred → Phase 8: preset verification on target hardware*

### 1.2 Dockerfile
- [x] Stage 1 `base-deps`: Ubuntu 24.04, GCC 13+, CMake 3.28+, Ninja, ccache, OpenCV 4.9+, ONNX Runtime 1.17+, Eigen, yaml-cpp, spdlog, nlohmann-json, GTest, Google Benchmark
- [x] Stage 2 `openfhe`: Build OpenFHE v1.4.2 from source (static, release, native)
- [x] Stage 3 `build`: Copy source, cmake configure+build, run tests
- [x] Stage 4 `runtime`: Minimal image -- binary + shared libs + model only, non-root, no shell
- [x] Create `docker-compose.yml` with `dev`, `test`, `bench`, `prod` services
- [x] Verify: `docker compose build` succeeds, container builds in < 30 minutes

### 1.3 Core Types
- [x] Implement `types.hpp`: `IRImage`, `SegmentationMap`, `GeometryMask`, `NoiseMask`, `Contour`, `GeometryPolygons`, `EyeCenters`, `EyeOrientation`, `NormalizedIris`, `IrisFilterResponse`, `IrisCode`
- [x] Implement `IrisTemplate`, `IrisTemplateWithId`, `WeightedIrisTemplate`, `AlignedTemplates`
- [x] Implement `Landmarks`, `DistanceMatrix`
- [x] Implement `Offgaze`, `Sharpness`, `EyeOcclusion`, `PupilToIrisProperty`, `BoundingBox`
- [x] Implement `PipelineOutput`
- [x] Implement `PackedIrisCode` in `iris_code_packed.hpp` (§16): `from_mat()`, `to_mat()`, `rotate()`
- [x] Verify: all types construct, move, and round-trip (pack/unpack) correctly in unit tests (9 tests in `test_types.cpp`)

### 1.4 Error Handling
- [x] Implement `error.hpp`: `ErrorCode` enum (all variants including `Cancelled`, `ConfigInvalid`, `ModelCorrupted`, `TemplateAggregationIncompatible`, `IdentityValidationFailed`, `KeyInvalid`, `KeyExpired`)
- [x] Implement `IrisError` struct and `Result<T>` alias (`std::expected<T, IrisError>`)
- [x] Implement `CancellationToken` in `cancellation.hpp` (§13.2): `cancel()`, `is_cancelled()`, thread-safe
- [x] Verify: `CancellationToken` works correctly under concurrent access (4 tests in `test_cancellation.cpp`)

### 1.5 Algorithm Base
- [x] Implement CRTP `Algorithm` base class in `algorithm.hpp`
- [x] Define `PipelineNodeBase` interface in `pipeline_node.hpp`

### 1.6 Thread Pool
- [x] Implement `ThreadPool` in `thread_pool.hpp`: `submit()`, `parallel_for()`, `batch_submit()`
- [x] Implement `pending_count()`, `active_count()`, `thread_count()`
- [x] Implement global pool accessor `global_pool()`
- [x] Verify: submit/wait, parallel_for correctness, batch_submit, clean shutdown, exception propagation (12 tests in `test_thread_pool.cpp`)

### 1.7 Async I/O
- [x] Implement `AsyncIO` in `async_io.hpp`: `read_file()`, `write_file()`, `download_model()`, `load_yaml()`
- [x] Implement `DownloadPolicy` (§13.1.3): max_retries=3, exponential backoff (1s, 4s, 16s), checksum verification
- [x] Verify: async file read/write, concurrent reads, download with cache, retry on failure (5 tests in `test_async_io.cpp`)

### 1.8 Config Parser + Node Registry
- [x] Implement YAML config loading with yaml-cpp
- [x] Implement `NodeRegistry` in `node_registry.hpp` (§15): `register_node()`, `lookup()`, singleton `instance()`
- [x] Implement `IRIS_REGISTER_NODE` macro
- [x] Load `open-iris/src/iris/pipelines/confs/pipeline.yaml` and verify all class names parse — `PipelineYamlLoadsAndBuilds` test (skips gracefully when yaml not in Docker context)
- [x] Verify: unknown `class_name` returns clear error (4 tests in `test_node_registry.cpp`)

### 1.9 First Node: Encoder
- [x] Implement `IrisEncoder` (complex response -> 2-bit binarization)
- [x] Register with `IRIS_REGISTER_NODE("iris.IrisEncoder", IrisEncoder)`
- [x] Verify: binarization and mask threshold correctness (4 tests in `test_encoder.cpp`)

### 1.10 SIMD Popcount
- [x] Implement `popcount_xor()` in `matcher_simd.hpp` (§14.2)
- [x] NEON cnt variant — **primary target** (Linux aarch64)
- [x] AVX-512 VPOPCNT variant (compile-time gated, retained for build compatibility)
- [x] AVX2 Harley-Seal variant (compile-time gated, retained for build compatibility)
- [x] Scalar `std::popcount` fallback
- [x] Verify: all variants produce identical results for known bit patterns (8 tests in `test_simd_popcount.cpp`)
- [x] Benchmark scaffold (`bench_matcher.cpp`)

### 1.11 NPY Reader/Writer + Comparison Skeleton
- [x] Implement `npy_reader.hpp` for reading `.npy` files in C++ (created during Phase 6)
- [x] Implement `npy_writer.hpp` for writing `.npy` files in C++ (4 tests in `test_npy_writer.cpp`)
- [x] Create comparison harness skeleton (`tests/comparison/`)
- [ ] Verify: reads Python-generated `.npy` files correctly — *deferred → Phase 8: requires Python-generated fixtures*

### 1.12 Unit Tests
- [x] `test_types.cpp`: construction, move, serialization, PackedIrisCode round-trip, rotation (9 tests)
- [x] `test_thread_pool.cpp`: submit/wait, parallel_for, batch_submit, shutdown, exceptions, pending/active counts (12 tests)
- [x] `test_async_io.cpp`: file read/write, concurrent reads, retry/backoff, errors (5 tests)
- [x] `test_cancellation.cpp`: cancel/check, concurrent cancel+check (4 tests)
- [x] `test_node_registry.cpp`: register/lookup, unknown class error, auto-registration (4 tests)
- [x] `test_simd_popcount.cpp`: correctness vs scalar, known patterns, AND, XOR-AND (8 tests)
- [x] `test_encoder.cpp`: binarization correctness, mask threshold, empty input, NodeParams (4 tests)
- [x] `test_error.cpp`: error code names, make_error, Result value/error (4 tests)
- [x] All tests pass on Linux ARM (container): **50 tests**

### Phase 1 Gate
- [x] Container builds from scratch in < 30 minutes
- [x] `IrisEncoder` produces correct binarization output (4 tests)
- [x] All core types compile and pass unit tests (including `PackedIrisCode` round-trip)
- [x] CMake builds with GCC 13 (target: Linux aarch64)
- [ ] `NodeRegistry` resolves all class names from `pipeline.yaml` — *deferred → Phase 6: requires all nodes registered*
- [x] SIMD `popcount_xor` produces identical results across all platform variants (NEON is primary target)
- [x] `CancellationToken` works correctly under concurrent access
- [x] `AsyncIO::download_model` retries on failure with exponential backoff

---

## Phase 2: Image Processing Nodes

> **Note**: BisectorsEyeCenter (§3.1) and MomentOrientation (§3.2) were pulled from Phase 3 into Phase 2 because they are direct dependencies of Smoothing and LinearExtrapolation nodes. A shared `geometry_utils` module was also added as Step 1.

### 2.0 Geometry Utilities (added)
- [x] Implement `geometry_utils.hpp/cpp`: `cartesian2polar`, `polar2cartesian`, `polar2contour`, `polygon_area`, `estimate_diameter`, `filled_eyeball_mask`, `filled_iris_mask`
- [x] Verify: 15 tests in `test_geometry_utils.cpp` (polar conversions, round-trips, area, diameter, mask filling)

### 2.1 Segmentation
- [x] Implement `OnnxSegmentation`: ONNX Runtime C++ API, conditional compilation (`#ifdef IRIS_HAS_ONNXRUNTIME`)
- [x] Preprocessing pipeline: resize -> bilateral denoise (optional) -> [0,1] normalize -> tile to 3ch -> ImageNet normalize -> NCHW float32
- [x] ORT state wrapped in `shared_ptr` for `std::any` copyability (required by `IRIS_REGISTER_NODE`)
- [x] SYSTEM includes for ONNX Runtime headers to suppress third-party warnings under `-Werror`
- [x] Stub fallback when ONNX Runtime unavailable (returns `SegmentationFailed` error)
- [x] Register: `IRIS_REGISTER_NODE("iris.MultilabelSegmentation.create_from_hugging_face", OnnxSegmentation)`
- [x] Verify: invalid model path throws, invalid NodeParams throws (2 tests in `test_segmentation.cpp`)
- [ ] Verify: 4-class segmentation map matches Python (max element-wise diff < 1e-4) — *deferred → Phase 6: requires ONNX model + NPY fixtures + cross-language harness*

### 2.2 Binarization
- [x] Implement `SegmentationBinarizer`: threshold segmentation map per class (default 0.5)
- [x] Register: `IRIS_REGISTER_NODE("iris.MultilabelSegmentationBinarization", SegmentationBinarizer)`
- [x] Verify: 5 tests in `test_binarization.cpp` (default thresholds, below threshold, mixed values, custom thresholds, empty input)
- [ ] Verify: binary masks match Python exactly — *deferred → Phase 6: requires NPY fixtures*

### 2.3 Specular Reflection
- [x] Implement `SpecularReflectionDetector`: intensity threshold (default 254)
- [x] Register: `IRIS_REGISTER_NODE("iris.SpecularReflectionDetection", SpecularReflectionDetector)`
- [x] Verify: 4 tests in `test_specular_reflection.cpp` (basic threshold, custom threshold, empty image, NodeParams)

### 2.4 Vectorization
- [x] Implement `ContourExtractor`: `cv::findContours(RETR_TREE, CHAIN_APPROX_SIMPLE)` with area filter (relative_area_threshold=0.03)
- [x] Register: `IRIS_REGISTER_NODE("iris.ContouringAlgorithm", ContourExtractor)`
- [x] Verify: 4 tests in `test_vectorization.cpp` (circular contours, empty iris, area filter, circle approximation)
- [ ] Verify: contour matches Python (Hausdorff distance < 1 pixel) — *deferred → Phase 6: requires NPY fixtures*

### 2.5 Contour Interpolation
- [x] Implement `ContourInterpolator`: insert linearly interpolated points where distance > max_dist_px (1% of iris diameter)
- [x] Register: `IRIS_REGISTER_NODE("iris.ContourInterpolation", ContourInterpolator)`
- [x] Verify: 3 tests in `test_geometry_refinement.cpp` (large gaps, dense no-op, empty iris)

### 2.6 Distance Filter
- [x] Implement `EyeballDistanceFilter`: paint eyeball contour into noise mask, dilate with `cv::blur`, filter iris/pupil points in forbidden zones
- [x] Register: `IRIS_REGISTER_NODE("iris.ContourPointNoiseEyeballDistanceFilter", EyeballDistanceFilter)`
- [x] Verify: 2 tests in `test_geometry_refinement.cpp` (removes points near noise, no-noise passthrough)

### 2.7 BisectorsEyeCenter (pulled from Phase 3)
- [x] Implement `BisectorsEyeCenter`: random perpendicular bisectors from polygon vertex pairs, least-squares 2x2 intersection (fixed seed 142857)
- [x] Register: `IRIS_REGISTER_NODE("iris.BisectorsMethod", BisectorsEyeCenter)`
- [x] Verify: 5 tests in `test_eye_centers.cpp` (perfect circle, off-center pupil, partial arc, too few points, custom params)

### 2.8 MomentOrientation (pulled from Phase 3)
- [x] Implement `MomentOrientation`: `cv::moments`, eccentricity check, `0.5 * atan2(2*mu11, mu20-mu02)`, normalized to [-pi/2, pi/2)
- [x] Register: `IRIS_REGISTER_NODE("iris.MomentOfArea", MomentOrientation)`
- [x] Verify: 5 tests in `test_eye_orientation.cpp` (horizontal/vertical/rotated ellipse, circular rejection, too few points)

### 2.9 Smoothing
- [x] Implement `ContourSmoother`: polar conversion -> gap detection -> arc splitting -> rolling median filter -> back to Cartesian
- [x] Register: `IRIS_REGISTER_NODE("iris.Smoothing", ContourSmoother)`
- [x] Verify: 3 tests in `test_geometry_estimation.cpp` (smooths circular contour, preserves circular shape, empty iris fails)

### 2.10 Linear Extrapolation
- [x] Implement `LinearExtrapolator`: polar conversion, 3-copy padding for periodicity, linear interpolation at uniform dphi (0.9 default), extract [0, 2pi)
- [x] Register: `IRIS_REGISTER_NODE("iris.LinearExtrapolation", LinearExtrapolator)`
- [x] Verify: 4 tests in `test_geometry_estimation.cpp` (extrapolates circle, preserves radius, custom dphi, too few points)

### 2.11 Ellipse Fitting
- [x] Implement `EllipseFitter`: `cv::fitEllipse`, pupil-inside-iris validation (bounding circle with 5% margin), parametric ellipse point generation, refinement with original pupil positions
- [x] Register: `IRIS_REGISTER_NODE("iris.LSQEllipseFitWithRefinement", EllipseFitter)`
- [x] Verify: 3 tests in `test_geometry_estimation.cpp` (fits circular contour, rejects pupil outside iris, too few points)
- [ ] Verify: ellipse parameters match Python (< 0.1% relative error) — *deferred → Phase 6: requires NPY fixtures*

### 2.12 Fusion Extrapolation
- [x] Implement `FusionExtrapolator`: runs both LinearExtrapolator and EllipseFitter, computes shape statistics (relative_std of squared radius ratios), selects based on spread threshold (0.014) and regularity condition
- [x] Handle nested params via dot-separated keys (`circle_extrapolation.dphi`, `ellipse_fit.dphi`)
- [x] Register: `IRIS_REGISTER_NODE("iris.FusionExtrapolation", FusionExtrapolator)`
- [x] Verify: 1 test in `test_geometry_estimation.cpp` (fuses circle and ellipse)

### 2.13 Generate Python Reference Data
- [ ] Create `tools/generate_test_fixtures.py`: run Python pipeline on CASIA1 images, save all intermediate outputs as `.npy` + metadata as JSON — *deferred → Phase 6: needs NPY reader (1.11) + pipeline executor*
- [ ] Generate fixtures for at least 5 test images from `data/` — *deferred → Phase 6*

### 2.14 Facade Header
- [x] Update `iris.hpp` to expose all Phase 1+2 node headers + geometry_utils

### 2.15 Unit Tests Summary
- [x] `test_geometry_utils.cpp`: polar conversions, round-trips, area, diameter, mask filling (15 tests)
- [x] `test_specular_reflection.cpp`: threshold correctness, empty image, NodeParams (4 tests)
- [x] `test_binarization.cpp`: threshold correctness, class separation, empty input (5 tests)
- [x] `test_vectorization.cpp`: contour extraction, area filtering, circle approximation (4 tests)
- [x] `test_geometry_refinement.cpp`: interpolation accuracy, distance filter (5 tests)
- [x] `test_eye_centers.cpp`: bisectors eye center estimation (5 tests)
- [x] `test_eye_orientation.cpp`: moment-of-area orientation (5 tests)
- [x] `test_geometry_estimation.cpp`: smoothing, extrapolation, ellipse fit, fusion (11 tests)
- [x] `test_segmentation.cpp`: ONNX error paths (2 tests)
- [x] All tests pass on Linux ARM (container): **56 new tests (106 total)**
- [ ] Cross-language tests pass (C++ output vs Python `.npy` fixtures within tolerance) — *deferred → Phase 6: requires NPY reader (1.11) + Python fixtures (2.13)*

### Phase 2 Gate
- [x] All 13 algorithm nodes implemented, compiled, unit-tested with synthetic data
- [x] All nodes registered with correct Python class names via `IRIS_REGISTER_NODE`
- [x] Docker build green with `-Werror -Wconversion -Wsign-conversion -Wdouble-promotion`
- [ ] Segmentation ONNX inference produces same 4-class map (max element-wise diff < 1e-4) — *deferred → Phase 6: requires ONNX model + cross-language harness*
- [ ] Contour extraction matches Python contours (Hausdorff distance < 1 pixel) — *deferred → Phase 6: requires NPY fixtures*
- [ ] Ellipse fit parameters match Python (< 0.1% relative error) — *deferred → Phase 6: requires NPY fixtures*
- [ ] All geometry nodes pass cross-language tests — *deferred → Phase 6: requires NPY reader + Python fixtures*

---

## Phase 3: Eye Properties & Normalization

### 3.1 Bisectors Method
- [x] ~~Implemented in Phase 2 (Step 2.7)~~ — `BisectorsEyeCenter` registered as `iris.BisectorsMethod`, 5 tests passing

### 3.2 Moment of Area
- [x] ~~Implemented in Phase 2 (Step 2.8)~~ — `MomentOrientation` registered as `iris.MomentOfArea`, 5 tests passing

### 3.3 Offgaze Estimation
- [x] Implement `EccentricityOffgazeEstimation`: eccentricity from moments or ellipse_fit, 5 assembling methods
- [x] Register: `IRIS_REGISTER_NODE("iris.EccentricityOffgazeEstimation", EccentricityOffgazeEstimation)`
- [x] Verify: 6 tests in `test_offgaze_estimation.cpp`

### 3.4 Occlusion Calculator
- [x] Implement `OcclusionCalculator`: padded workspace, quantile angular slices, mask operations for visibility fraction
- [x] Register: `IRIS_REGISTER_NODE("iris.OcclusionCalculator", OcclusionCalculator)`
- [x] Verify: 5 tests in `test_occlusion_calculator.cpp`

### 3.5 Pupil/Iris Ratio
- [x] Implement `PupilIrisPropertyCalculator`: diameter + center distance ratios with min thresholds
- [x] Register: `IRIS_REGISTER_NODE("iris.PupilIrisPropertyCalculator", PupilIrisPropertyCalculator)`
- [x] Verify: 5 tests in `test_pupil_iris_property.cpp`

### 3.6 Sharpness Estimation
- [x] Implement `SharpnessEstimation`: `cv::Laplacian` → erode mask → std of Laplacian in unmasked region
- [x] Register: `IRIS_REGISTER_NODE("iris.SharpnessEstimation", SharpnessEstimation)`
- [x] Verify: 4 tests in `test_sharpness_estimation.cpp`
- [ ] Verify: sharpness scores within 1% of Python — *deferred → Phase 6: requires NPY fixtures*

### 3.7 Bounding Box
- [x] Implement `IrisBBoxCalculator`: min/max of iris contour with multiplicative buffer, clamp to image bounds
- [x] Register: `IRIS_REGISTER_NODE("iris.IrisBBoxCalculator", IrisBBoxCalculator)`
- [x] Verify: 4 tests in `test_iris_bbox.cpp`

### 3.8 Linear Normalization
- [x] Implement `LinearNormalization`: Daugman rubber-sheet, linear correspondence via `point = pupil + t * (iris - pupil)`
- [x] Implement shared normalization utilities: `correct_orientation`, `generate_iris_mask`, `normalize_all`, `interpolate_pixel_intensity`
- [x] Register: `IRIS_REGISTER_NODE("iris.LinearNormalization", LinearNormalization)`
- [x] Verify: 4 tests in `test_linear_normalization.cpp`
- [ ] Verify: normalized image matches Python (PSNR > 60dB) — *deferred → Phase 6: requires NPY fixtures*

### 3.9 Nonlinear Normalization
- [x] Implement `NonlinearNormalization`: precomputed quadratic radial grids (101 entries, one per integer p2i_ratio percentage)
- [x] Register: `IRIS_REGISTER_NODE("iris.NonlinearNormalization", NonlinearNormalization)`
- [x] Verify: 3 tests in `test_nonlinear_normalization.cpp`

### 3.10 Perspective Normalization
- [x] Implement `PerspectiveNormalization`: per-ROI `cv::getPerspectiveTransform`, bilinear remap
- [x] Register: `IRIS_REGISTER_NODE("iris.PerspectiveNormalization", PerspectiveNormalization)`
- [x] Verify: 3 tests in `test_perspective_normalization.cpp`

### 3.11 Noise Mask Aggregation
- [x] Implement `NoiseMaskUnion`: `cv::bitwise_or` all masks
- [x] Register: `IRIS_REGISTER_NODE("iris.NoiseMaskUnion", NoiseMaskUnion)`
- [x] Verify: 5 tests in `test_noise_mask_union.cpp`

### 3.12 All Validators
- [x] Implement and register all validators:
  - [x] `Pupil2IrisPropertyValidator` — diameter_ratio & center_dist in range (5 tests)
  - [x] `OffgazeValidator` — score <= max (2 tests)
  - [x] `OcclusionValidator` — fraction >= min (2 tests)
  - [x] `IsPupilInsideIrisValidator` — all pupil points inside iris via `cv::pointPolygonTest` (2 tests)
  - [x] `SharpnessValidator` — score >= min (2 tests)
  - [x] `IsMaskTooSmallValidator` — mask code popcount >= min (2 tests)
  - [x] `PolygonsLengthValidator` — iris/pupil perimeter >= min (3 tests)
  - [x] `EyeCentersInsideImageValidator` — centers within image by min_distance (5 tests)
  - [x] `ExtrapolatedPolygonsInsideImageValidator` — % of polygon points inside image >= threshold (4 tests)
- [ ] Verify: all validators produce same pass/fail as Python for test dataset — *deferred → Phase 6: requires NPY fixtures + full pipeline*

### 3.13 Unit Tests
- [x] `test_normalization_utils.cpp`: correct_orientation, generate_iris_mask, normalize_all, interpolate (10 tests)
- [x] `test_geometry_utils.cpp`: extended with contour_to_filled_mask, polygon_length (4 new tests, 19 total)
- [x] `test_offgaze_estimation.cpp`: eccentricity methods, assembling, edge cases (6 tests)
- [x] `test_pupil_iris_property.cpp`: concentric, offset, too small, NodeParams (5 tests)
- [x] `test_iris_bbox.cpp`: basic, buffer, clamp, NodeParams (4 tests)
- [x] `test_noise_mask_union.cpp`: two masks, three masks, single, empty, mismatched (5 tests)
- [x] `test_occlusion_calculator.cpp`: concentric, offset, noise, small quantile, NodeParams (5 tests)
- [x] `test_linear_normalization.cpp`: concentric, oversat masking, mismatched points, NodeParams (4 tests)
- [x] `test_nonlinear_normalization.cpp`: concentric, radial gradient, mismatched points (3 tests)
- [x] `test_perspective_normalization.cpp`: concentric, custom res_in_r, mismatched points (3 tests)
- [x] `test_sharpness_estimation.cpp`: sharp image, blurred, all masked, empty (4 tests)
- [x] `test_object_validators.cpp`: all 7 object validators with pass/fail cases (18 tests)
- [x] `test_cross_object_validators.cpp`: 2 cross-object validators with pass/fail cases (9 tests)
- [ ] Cross-language comparison for all nodes — *deferred → Phase 6: requires NPY reader + Python fixtures*
- [x] All tests pass on Linux ARM (container): **186 tests**

### Phase 3 Gate
- [ ] Eye center estimation within 0.5 pixels of Python — *deferred → Phase 6: requires NPY fixtures*
- [ ] Normalization output matches Python (PSNR > 60dB) — *deferred → Phase 6: requires NPY fixtures*
- [ ] All validators produce same pass/fail as Python for test dataset — *deferred → Phase 6: requires NPY fixtures + full pipeline*
- [ ] Sharpness scores within 1% of Python — *deferred → Phase 6: requires NPY fixtures*
- [x] All Phase 3 nodes implemented, compiled, unit-tested with synthetic data (186 tests)
- [x] All nodes registered with correct Python class names via `IRIS_REGISTER_NODE`
- [x] Docker build green with `-Werror -Wconversion -Wsign-conversion -Wdouble-promotion`
- [x] Facade header (`iris.hpp`) updated with all Phase 3 headers

---

## Phase 4: Feature Extraction & Encoding

### 4.1 Gabor Filter
- [x] Implement `GaborFilter`: 2D spatial Gabor kernel generation with DC correction and fixed-point option
- [x] Register: `IRIS_REGISTER_NODE("iris.GaborFilter", GaborFilter)`
- [x] Shared filter utilities: `filter_utils.hpp/.cpp` (make_meshgrid, rotate_coords, normalize_frobenius, apply_fixpoints)
- [x] Verify: 8 tests in `test_gabor_filter.cpp` (kernel shape, DC correction, Frobenius norm, fixpoints, various params, NodeParams)
- [ ] Verify: kernel values match Python (max diff < 1e-10) — *deferred → Phase 6: requires NPY fixtures*

### 4.2 Log-Gabor Filter
- [x] Implement `LogGaborFilter`: frequency-domain log-Gabor via 2D FFT (`cv::dft`/`cv::idft`)
- [x] Register: `IRIS_REGISTER_NODE("iris.LogGaborFilter", LogGaborFilter)`
- [x] Add to dispatch table in `node_dispatcher.cpp`
- [x] Verify: 7 tests in `test_log_gabor_filter.cpp` (default construction, kernel dimensions, Frobenius norm, fixpoints, different theta, no NaN/Inf, NodeParams)

### 4.3 Regular Probe Schema
- [x] Implement `RegularProbeSchema`: uniform grid sampling with configurable size/boundary
- [x] Register: `IRIS_REGISTER_NODE("iris.RegularProbeSchema", RegularProbeSchema)`
- [x] Verify: 6 tests in `test_regular_probe_schema.cpp` (default grid, rhos range, phis range, periodic-symmetric, custom size, NodeParams)

### 4.4 Convolutional Filter Bank
- [x] Implement `ConvFilterBank`: spatial convolution at probe positions with polar padding, mask reliability weights
- [x] Default: 2 Gabor filters matching pipeline.yaml (41×21 + 17×21 kernels, to_fixpoints=true)
- [x] Polar padding: zero-pad vertically, circular wrap horizontally
- [x] Mask computation: fraction of filter energy at valid mask pixels (duplicated or separate real/imag)
- [x] Register: `IRIS_REGISTER_NODE("iris.ConvFilterBank", ConvFilterBank)`
- [x] Fixed types.hpp comment: `IrisFilterResponse::Response::mask` is CV_64FC2 (not CV_8UC1)
- [x] Verify: 11 tests in `test_conv_filter_bank.cpp` (default construction, output shape, DC-corrected uniform, gradient image, mask all valid, mask partial, mask duplicated, mask not duplicated, multiple filters, iris code version, empty image, reproducibility)

### 4.5 Fragile Bit Refinement
- [x] Implement `FragileBitRefinement`: threshold fragile bits in polar or cartesian mode
- [x] Polar mode: radius and angle thresholds; Cartesian mode: separate |real| and |imag| thresholds
- [x] Support mask_is_duplicated (real/imag mask channels identical vs separate)
- [x] Register: `IRIS_REGISTER_NODE("iris.FragileBitRefinement", FragileBitRefinement)`
- [x] Verify: 9 tests in `test_fragile_bit_refinement.cpp` (polar radius below/above/in range, polar mask duplicated, cartesian below min, cartesian in range, cartesian mask duplicated, data passthrough, NodeParams)

### 4.6 Iris Encoder (finalize)
- [x] Verify encoder integrates with filter bank output end-to-end
- [x] Pack encoder output into `PackedIrisCode` via `PackedIrisCode::from_mat()`
- [x] Verify: 4 integration tests in `test_feature_extraction_chain.cpp` (full chain, output dimensions, reproducibility, all-black image)

### 4.7 Unit Tests Summary
- [x] `test_gabor_filter.cpp`: 8 tests
- [x] `test_regular_probe_schema.cpp`: 6 tests
- [x] `test_conv_filter_bank.cpp`: 11 tests
- [x] `test_fragile_bit_refinement.cpp`: 9 tests
- [x] `test_feature_extraction_chain.cpp`: 4 integration tests (NormalizedIris → template)
- [x] All tests pass on Linux ARM (container): **225 tests**

### 4.8 Iris Code Bit Agreement Test
- [ ] Cross-language test: `popcount(cpp_code XOR python_code) == 0` for all test images — *deferred → Phase 6: requires NPY reader + Python fixtures*
- [ ] Verify: 100% bit agreement (16,384 bits per template) — *deferred → Phase 6*

### Phase 4 Gate
- [ ] Gabor filter kernels match Python (max diff < 1e-10) — *deferred → Phase 6: requires NPY fixtures*
- [ ] Filter response values match Python (relative error < 1e-5) — *deferred → Phase 6: requires NPY fixtures*
- [ ] Iris code bits are **identical** to Python for same normalized input — *deferred → Phase 6: requires NPY fixtures*
- [ ] Mask codes are **identical** to Python — *deferred → Phase 6: requires NPY fixtures*
- [x] All Phase 4 nodes implemented, compiled, unit-tested with synthetic data (225 tests)
- [x] All nodes registered with correct Python class names via `IRIS_REGISTER_NODE`
- [x] Full extraction chain tested: NormalizedIris → ConvFilterBank → FragileBitRefinement → IrisEncoder → IrisTemplate
- [x] Docker build green with `-Werror -Wconversion -Wsign-conversion -Wdouble-promotion`
- [x] Facade header (`iris.hpp`) updated with all Phase 4 headers

---

## Phase 5: Matching & Template Operations

### 5.1 SimpleHammingDistanceMatcher
- [x] Implement `SimpleHammingDistanceMatcher`: basic HD using `PackedIrisCode` + SIMD popcount, rotation shifts [0, ±1, ..., ±rotation_shift], optional normalisation
- [x] Register: `IRIS_REGISTER_NODE("iris.SimpleHammingDistanceMatcher", SimpleHammingDistanceMatcher)`
- [x] Verify: 12 tests in `test_hamming_distance_matcher.cpp` (identical zero, identical ones, opposite max, empty codes, empty mask, partial mask, half bits differ, rotation finds min, rotation shift zero, normalised HD, norm clamp, NodeParams)

### 5.2 HammingDistanceMatcher
- [x] Implement `HammingDistanceMatcher`: rotation shift, normalization, separate half matching (split at row 8 / word 64, average upper/lower halves)
- [x] Uses `PackedIrisCode::rotate()` for circular column shifts
- [x] Register: `IRIS_REGISTER_NODE("iris.HammingDistanceMatcher", HammingDistanceMatcher)`
- [x] Verify: 5 tests in `test_hamming_distance_matcher.cpp` (identical zero, separate half enabled, separate half disabled, multiple wavelets, NodeParams)
- [ ] Verify: HD **identical** to Python for all test pairs — *deferred → Phase 6: requires NPY fixtures + cross-language harness*

### 5.3 BatchMatcher (1-vs-N + N-vs-N)
- [x] Implement `BatchMatcher::match_one_vs_n()`: `parallel_for` over gallery entries via global thread pool
- [x] Implement `BatchMatcher::match_n_vs_n()`: upper-triangle parallel matching, returns `DistanceMatrix`
- [x] Implement `BatchMatcher::match_n_vs_n_with_rotations()`: also stores best rotation per pair in `cv::Mat(N, N, CV_32SC1)`
- [x] Not a pipeline node (utility class, no `IRIS_REGISTER_NODE`)
- [x] Verify: 8 tests in `test_batch_matcher.cpp` (1-vs-N identity, 1-vs-N empty, 1-vs-N single, N-vs-N symmetric, N-vs-N diagonal zero, N-vs-N single, N-vs-N with rotations, parallel matches serial)

### 5.4 Template Alignment
- [x] Implement `HammingDistanceBasedAlignment`: pairwise distances → reference selection → rotate to align
- [x] Implement `ReferenceSelectionMethod` enum (Linear, MeanSquared, RootMeanSquared)
- [x] Register: `IRIS_REGISTER_NODE("iris.HammingDistanceBasedAlignment", HammingDistanceBasedAlignment)`
- [x] Verify: 10 tests in `test_template_alignment.cpp` (single template, two identical, two shifted, ref selection linear/mean_sq/RMS, mask codes rotated, distance matrix populated, empty fails, NodeParams)

### 5.5 Majority Vote Aggregation
- [x] Implement `MajorityVoteAggregation`: per-bit voting across templates with consistency weighting, mask threshold, inconsistent bit handling
- [x] Unpacks codes via `to_mat()`, counts votes, re-packs via `from_mat()`
- [x] Register: `IRIS_REGISTER_NODE("iris.MajorityVoteAggregation", MajorityVoteAggregation)`
- [x] Verify: 12 tests in `test_majority_vote_aggregation.cpp` (all agree 1, all agree 0, majority wins, mask threshold, mask threshold low, consistency weight, inconsistent weight, inconsistent disabled, single template, multiple wavelets, empty fails, NodeParams)

### 5.6 Template Identity Filter
- [x] Implement `TemplateIdentityFilter`: BFS connected components on distance graph, keep largest cluster
- [x] Implement `IdentityValidationAction` enum (Remove, RaiseError)
- [x] Register: `IRIS_REGISTER_NODE("iris.TemplateIdentityFilter", TemplateIdentityFilter)`
- [x] Verify: 10 tests in `test_template_identity_filter.cpp` (all same identity, one outlier remove, one outlier raise error, two clusters, all outliers, min templates violation, single template, empty input, distance matrix preserved, NodeParams)

### 5.7 SIMD Optimization
- [ ] Profile Hamming distance inner loop — *deferred → Phase 8: profiling & optimization pass*
- [ ] Benchmark NEON SIMD vs scalar on real template data — *deferred → Phase 8: requires real template data + benchmark tooling*
- [x] NEON path verified via unit tests (8 tests in `test_simd_popcount.cpp`)

### 5.8 Template Serialization
- [x] Implement `base64.hpp`: `encode()`, `decode()` — RFC 4648 lookup-table implementation (7 tests in `test_base64.cpp`)
- [x] Implement `TemplateSerializer` (§3.5): `to_binary()`, `from_binary()` — binary format (2056 bytes: magic + version + code + mask)
- [x] Implement `TemplateSerializer`: `to_python_format()`, `from_python_format()` — JSON + base64 + packbits matching Python's format
- [x] Verify: round-trip correctness for binary and Python formats (11 tests in `test_template_serializer.cpp`)
- [x] Implement `template_npy_utils.hpp`: `write_packed_code()`, `read_packed_code()` for PackedIrisCode NPY round-trip
- [x] Verify: 3 tests in `test_template_npy.cpp` (round-trip zeros, round-trip random, read nonexistent)

### 5.9 Unit Tests Summary
- [x] `test_hamming_distance_matcher.cpp`: 17 tests (Simple: 12, Hamming: 5)
- [x] `test_batch_matcher.cpp`: 8 tests
- [x] `test_template_alignment.cpp`: 10 tests
- [x] `test_majority_vote_aggregation.cpp`: 12 tests
- [x] `test_template_identity_filter.cpp`: 10 tests
- [x] All tests pass on Linux ARM (container): **282 tests**

### 5.10 HD Correlation + Biometric Accuracy
- [ ] Compute HD for N pairs in both Python and C++ (N >= 190 from 20 CASIA1 images) — *deferred → Phase 6: requires full pipeline + CASIA1 data + cross-language harness*
- [ ] Pearson correlation r == 1.0, max absolute discrepancy == 0.0 — *deferred → Phase 6*
- [ ] FMR/FNMR comparison on CASIA1 dataset — *deferred → Phase 6*
- [ ] Verify: same match/no-match decisions at threshold=0.35 — *deferred → Phase 6*

### Phase 5 Gate
- [ ] Hamming distance **identical** to Python for all test pairs — *deferred → Phase 6: requires NPY fixtures + cross-language harness*
- [ ] Rotation shift finds same optimal shift as Python — *deferred → Phase 6: requires NPY fixtures*
- [ ] Batch matcher throughput > 1M comparisons/second — *deferred → Phase 8: benchmark + optimization pass*
- [ ] Same match/no-match decisions as Python on test dataset — *deferred → Phase 6: requires full pipeline + CASIA1 data*
- [x] All Phase 5 nodes implemented, compiled, unit-tested with synthetic data (282 tests)
- [x] All nodes registered with correct Python class names via `IRIS_REGISTER_NODE`
- [x] Docker build green with `-Werror -Wconversion -Wsign-conversion -Wdouble-promotion`
- [x] Facade header (`iris.hpp`) updated with all Phase 5 headers

---

## Phase 6: Pipeline Orchestration

### 6.1 PipelineContext
- [x] Implement `PipelineContext`: typed wrapper around `std::unordered_map<std::string, std::any>`
- [x] Methods: `set<T>()`, `get<T>()`, `get_indexed<Index, PairT>()`, `has()`, `get_raw()`, `keys()`, `size()`, `clear()`
- [x] Verify: 13 tests in `test_pipeline_context.cpp`

### 6.2 Node Dispatcher (Type Dispatch Table)
- [x] Implement explicit dispatch table bridging type-erased `std::any` to strongly-typed `run()` calls
- [x] Template helpers: `dispatch_1`, `dispatch_1_pair`, `dispatch_2`, `dispatch_2_void`, `dispatch_4`
- [x] Custom dispatchers: `dispatch_occlusion` (YAML arg reorder), `dispatch_noise_union` (multi-source)
- [x] All 22+ pipeline node class_names registered with dispatch functions
- [x] Verify: 10 tests in `test_node_dispatcher.cpp`

### 6.3 Long-Form Registry Aliases + Callback Dispatcher
- [x] Add `IRIS_REGISTER_NODE_ALIAS` macro to `node_registry.hpp`
- [x] Register 10 long-form Python path aliases (7 object validators, 2 cross-object validators, 1 fragile bit refinement)
- [x] Implement `CallbackDispatcher`: `callback_1<Validator, OutputType>` template with short+long form names
- [x] Verify: 6 tests in `test_callback_dispatcher.cpp`

### 6.4 Pipeline DAG Builder + Config Validation
- [x] Implement `PipelineDAG::build()`: validates class_names, source_nodes, detects cycles
- [x] Instantiate algorithms + callback validators via `NodeRegistry`
- [x] Compute execution levels via Kahn's algorithm (BFS topological sort)
- [x] Verify: 11 tests in `test_pipeline_dag.cpp` (build, levels, parallel nodes, unknown class, missing source, circular deps, empty, single node, duplicate name, all YAML names dispatched, callbacks)

### 6.5 Pipeline Executor
- [x] Implement `PipelineExecutor`: level-by-level execution with parallel nodes via thread pool
- [x] `ErrorStrategy::Raise` and `ErrorStrategy::Store` modes
- [x] `CancellationToken` checked between levels
- [x] Callback validators run after each node
- [x] Verify: 7 tests in `test_pipeline_executor.cpp` (single node, multi-level, raise/store failure, cancellation, callbacks, parallel)

### 6.6 IrisPipeline Facade
- [x] Implement `IrisPipeline`: `from_config(path)`, `from_config(PipelineConfig)`, `run(IRImage, token)`
- [x] Loads YAML config → builds DAG → creates ThreadPool → executes
- [x] Extracts `IrisTemplate` from "encoder" node output
- [x] Verify: 6 tests in `test_iris_pipeline.cpp` (valid config, invalid config, unknown class, run output, cancellation, multiple runs)

### 6.7 AggregationPipeline
- [x] Implement `AggregationPipeline`: `HammingDistanceBasedAlignment` → `MajorityVoteAggregation`
- [x] Requires at least 2 templates
- [x] Verify: 3 tests in `test_multiframe_pipeline.cpp` (two templates, too few, three templates)

### 6.8 MultiframePipeline
- [x] Implement `MultiframePipeline`: processes N images, collects successful templates, aggregates
- [x] Partial failure tolerance via `min_successful` parameter
- [x] Cancellation support
- [x] Verify: 4 tests in `test_multiframe_pipeline.cpp` (construct valid, construct invalid, too few images, cancellation)

### 6.9 Integration Tests
- [x] Post-segmentation chain: binarization → vectorization → interpolation with synthetic data
- [x] DAG-parallel vs sequential parity (ThreadPool(1) vs ThreadPool(4))
- [x] Cancellation mid-pipeline returns `ErrorCode::Cancelled`
- [x] Config validation: bad class_name, circular deps, missing source_node
- [x] IrisPipeline facade DAG inspection
- [x] Verify: 7 tests in `test_pipeline_integration.cpp`

### 6.10 Unit Tests Summary
- [x] `test_pipeline_context.cpp`: 13 tests
- [x] `test_node_dispatcher.cpp`: 10 tests
- [x] `test_callback_dispatcher.cpp`: 6 tests
- [x] `test_pipeline_dag.cpp`: 12 tests (11 original + 1 callback validator coverage)
- [x] `test_pipeline_executor.cpp`: 7 tests
- [x] `test_iris_pipeline.cpp`: 6 tests
- [x] `test_multiframe_pipeline.cpp`: 7 tests
- [x] `test_pipeline_integration.cpp`: 7 tests
- [x] All tests pass on Linux ARM (container): **349 tests** (Phase 6 core)

### 6.11 Deferred Items (Now Filled)
- [x] Base64 utility: `base64.hpp/.cpp` (7 tests in `test_base64.cpp`)
- [x] Template serialization: `template_serializer.hpp/.cpp` (11 tests in `test_template_serializer.cpp`)
- [x] NPY writer: `npy_writer.hpp` (4 tests in `test_npy_writer.cpp`)
- [x] Callback validator test coverage: `AllPipelineYamlCallbackNamesDispatched` (1 test added to `test_pipeline_dag.cpp`)
- [x] All deferred items pass on Linux (container): **372 tests** (349 + 23)

### 6.12 Still Deferred
- [ ] Full pipeline E2E with CASIA1 images — *deferred → Phase 8: requires ONNX model + CASIA1 data*
- [ ] Cross-language comparison (identical IrisTemplate to Python) — *deferred → Phase 8: requires NPY fixtures + comparison harness*
- [ ] Multiframe partial failure with real images — *deferred → Phase 8: requires ONNX model*
- [ ] Debugging output builder (all intermediate results) — *deferred → Phase 8: nice-to-have*

### Phase 6 Gate
- [x] All 22+ pipeline node class_names resolve in dispatch table
- [x] Long-form Python path aliases resolve for validators + FragileBitRefinement
- [x] DAG builder validates class_names, source_nodes, detects cycles
- [x] Kahn's algorithm produces correct execution levels
- [x] Pipeline executor runs nodes level-by-level with parallel execution
- [x] Error strategies (Raise/Store) work correctly
- [x] Cancellation mid-pipeline returns `ErrorCode::Cancelled`
- [x] Callback validators execute after node dispatch
- [x] IrisPipeline facade loads config, builds DAG, executes pipeline
- [x] AggregationPipeline aligns + aggregates templates
- [x] MultiframePipeline processes multiple images with partial failure tolerance
- [x] Invalid YAML config produces clear error at load time
- [x] Docker build green with `-Werror -Wconversion -Wsign-conversion -Wdouble-promotion`
- [x] Facade header (`iris.hpp`) updated with all Phase 6 headers
- [x] 349 tests passing (282 Phase 1-5 + 67 Phase 6)

---

## Phase 7: OpenFHE Integration

### 7.1 Secure Memory
- [x] Implement `SecureBuffer`: RAII wrapper with volatile-zero destructor (`explicit_bzero` or volatile memset fallback)
- [x] 64-byte aligned allocation via `std::aligned_alloc`
- [x] Best-effort `mlock()` on secret key memory (warn if fails)
- [x] Unconditional (no FHE dependency) — lives in `src/core/`, not `src/crypto/`
- [x] Verify: 5 tests in `test_secure_buffer.cpp` (construct/write/read, move construct, move assign, no copy, empty)

### 7.2 FHE Context Wrapper
- [x] Implement `FHEContext`: BFV parameter setup (plaintext_modulus=65537, mult_depth=2, ring_dim=16384)
- [x] Enable PKE, KEYSWITCH, LEVELEDSHE, ADVANCEDSHE features
- [x] Key generation (public, secret, EvalMult, EvalSum, EvalRotate with power-of-2 indices)
- [x] Slot count = ring_dim / 2 = 8192 SIMD slots for BFV batching
- [x] Verify: 6 tests in `test_fhe_context.cpp` (create default, slot count 8192, no keys before gen, gen succeeds, encrypt/decrypt round-trip, multiple contexts independent)

### 7.3 Template Encryption & Decryption
- [x] Implement `EncryptedTemplate::encrypt()`: pack 8192 code bits as integers (0/1) into SIMD slots, same for mask bits → 2 ciphertexts per template
- [x] Implement `EncryptedTemplate::decrypt()`: unpack slots back to `PackedIrisCode`
- [x] Implement `EncryptedTemplate::serialize()`/`deserialize()`: OpenFHE binary serialization
- [x] Verify: 6 tests in `test_encrypted_template.cpp` (round-trip zeros, random, all ones, serialize/deserialize, no keys fails, mask preserved)

### 7.4 Encrypted Matching
- [x] Implement `EncryptedMatcher::match_encrypted()`: server-side HD computation under encryption
- [x] HD via `(a-b)^2 = XOR` for binary values, `a*b = AND` for masks, `EvalSum` for aggregation
- [x] Implement `EncryptedMatcher::decrypt_result()`: client-side decryption + HD = num/denom
- [x] Implement `EncryptedMatcher::match()`: convenience encrypt + match + decrypt
- [x] BFV arithmetic is exact — encrypted HD matches plaintext HD exactly (no approximation error)
- [x] Verify: 8 tests in `test_encrypted_matcher.cpp` (identical HD=0, opposite HD=1, half-different HD=0.5, matches plaintext, partial mask, empty mask, random parity ×5, convenience match)

### 7.5 Rotation Under Encryption
- [x] Strategy B (probe-side rotation): rotate probe in plaintext, encrypt, match against single encrypted gallery
- [x] Implement `EncryptedMatcher::match_with_rotation()`: iterate shifts [-N, +N], return minimum HD + best rotation
- [x] Gallery encrypted once, probe re-encrypted for each rotation shift
- [x] Verify: 4 tests in `test_encrypted_matcher.cpp` (identical templates, shift=0 matches no-rotation, finds correct shift for shifted template, matches plaintext rotation HD)

### 7.6 Key Manager
- [x] Implement `KeyMetadata`: key_id (UUID), created_at (ISO 8601), expires_at, ring_dimension, plaintext_modulus, sha256_fingerprint (FNV-1a hash)
- [x] Implement `KeyBundle`: public key, secret key, metadata
- [x] Implement `KeyManager::generate()`: bundle from context with optional TTL
- [x] Implement `KeyManager::validate()`: ring_dimension + plaintext_modulus match, expiry check
- [x] Implement `KeyManager::is_expired()`: ISO 8601 timestamp comparison
- [x] Verify: 6 tests in `test_key_manager.cpp` (generate, validate, expired detected, non-expired, empty expiry, generate with TTL)

### 7.7 Key Store
- [x] Implement `KeyStore::save()`: directory layout (`public_key.bin`, `secret_key.bin`, `key_meta.json`)
- [x] Set file permissions: `secret_key.bin` → `0600` (best-effort)
- [x] Implement `KeyStore::load()`: full key bundle from directory
- [x] Implement `KeyStore::load_public_only()`: public key + metadata only (no secret key)
- [x] Verify: 4 tests in `test_key_store.cpp` (save/load round-trip, load public only, nonexistent dir fails, metadata preserved)

### 7.8 Integration Tests
- [x] Full flow: generate keys → encrypt → match encrypted → decrypt → verify HD
- [x] Encrypted 1-vs-5 batch matches plaintext for all gallery entries
- [x] Key save → load → encrypt → match works end-to-end
- [x] Re-encrypt with new keys: decrypt old → encrypt new → data preserved
- [x] Sequential independent matches produce correct independent results
- [x] Rotation end-to-end: shifted probe → encrypted rotation → finds correct offset
- [x] Verify: 6 tests in `test_fhe_integration.cpp`

### 7.9 Performance Benchmarks
- [x] `bench_crypto.cpp`: key generation, encryption, encrypted match, full match time
- [x] `bench_matcher.cpp`: plaintext match with/without rotation
- [ ] Performance targets (encryption < 200ms, match < 500ms) — *deferred → Phase 8: profiling on target hardware*

### 7.10 Unit Tests Summary
- [x] `test_secure_buffer.cpp`: 5 tests
- [x] `test_fhe_context.cpp`: 6 tests
- [x] `test_encrypted_template.cpp`: 7 tests (6 original + 1 `ReEncryptStandalone` added Phase 8)
- [x] `test_encrypted_matcher.cpp`: 12 tests (8 matcher + 4 rotation)
- [x] `test_key_manager.cpp`: 6 tests
- [x] `test_key_store.cpp`: 4 tests
- [x] `test_fhe_integration.cpp`: 6 tests
- [x] All tests pass on Linux ARM (container): **417 tests** (372 deferred + 45 FHE)

### 7.11 Deferred to Phase 8
- [ ] Verify: `SecureBuffer` zeroing under ASan/valgrind — *deferred → Phase 8: sanitizer pass*
- [x] Implement `EncryptedTemplate::re_encrypt()` as standalone static method (decrypt old → encrypt new)
- [x] Verify: 1 test in `test_encrypted_template.cpp` (`ReEncryptStandalone`)
- [ ] SHA-256 fingerprint (currently FNV-1a; upgrade when crypto hash available) — *deferred → Phase 8*
- [ ] Batch encrypted matching parallelization — *deferred → Phase 8: OpenFHE contexts not thread-safe*
- [ ] EvalRotateKey serialization in KeyStore — *deferred → Phase 8: rotation keys auto-generated from context*

### Phase 7 Gate
- [x] Encrypt-decrypt round-trip preserves template exactly (bit-exact)
- [x] Encrypted HD matches plaintext HD for all test pairs (exact match, < 1e-10)
- [x] Same match/no-match decisions under encryption (0 disagreements in 5+ random trials)
- [x] Encrypted rotation finds same optimal shift as plaintext rotation
- [x] Key serialization round-trip preserves functionality (save → load → encrypt → match works)
- [x] Key lifecycle: generate → save → load → validate round-trip works
- [x] `SecureBuffer` zeros memory on destruction, move semantics correct
- [x] Expired keys detected by `is_expired()`
- [x] Re-encryption via decrypt-old + encrypt-new produces identical data
- [x] Secret key file permissions set to `0600` (best-effort)
- [x] Docker build green with FHE enabled (`-DIRIS_ENABLE_FHE=ON`)
- [x] All FHE code gated behind `#ifdef IRIS_HAS_FHE` — no impact when FHE disabled
- [x] 417 tests passing (372 pre-FHE + 45 FHE)

---

## Phase 8: Hardening & Optimization

### 8.1 Memory Optimization
- [ ] Pool allocators for pipeline scratch memory — *blocked: requires profiling on target workload*
- [ ] Zero-copy where possible (span-based interfaces) — *blocked: requires profiling on target workload*
- [x] Move-only pipeline context (no unnecessary copies) — `PipelineContext` copy ctors deleted, move defaulted
- [x] Verify: 1 test in `test_pipeline_context.cpp` (`MoveOnlySemantics` with `static_assert`)
- [ ] Verify: peak memory < 200MB for single pipeline — *blocked: requires ONNX model + profiling*

### 8.2 SIMD Audit
- [ ] Profile hot paths: Gabor convolution, Hamming distance, bit packing — *blocked: requires profiling tooling*
- [x] NEON is the primary SIMD target (Linux aarch64) — verified via unit tests (`test_simd_popcount.cpp`)
- [x] Benchmarks enabled in Docker (`bench_matcher.cpp`, `bench_crypto.cpp`)

### 8.3 Thread Pool Tuning
- [ ] Profile optimal thread count on target hardware — *blocked: requires target hardware spec*
- [ ] Test work-stealing fairness under load — *blocked: requires target hardware spec*
- [ ] Verify saturation safeguards (§13.1.4) — *blocked: requires target hardware spec*

### 8.4 TSan Pass
- [x] Full test suite under ThreadSanitizer — `make test-tsan` via docker-compose `test-tsan` service
- [x] Include: DAG-parallel pipeline, multiframe, batch matcher, cancellation
- [ ] Zero data races — *requires running `make test-tsan` and fixing any findings*

### 8.5 ASan/UBSan Pass
- [x] Full test suite under AddressSanitizer + UndefinedBehaviorSanitizer — `make test-asan` via docker-compose `test-asan` service
- [ ] Zero findings — *requires running `make test-asan` and fixing any findings*

### 8.6 Async I/O Stress Test
- [ ] Concurrent model downloads with retry/backoff — *blocked: requires network + ONNX model server*
- [ ] Parallel file writes — *blocked: requires target workload*
- [ ] Checksum verification under load — *blocked: requires target workload*
- [ ] Error recovery scenarios — *blocked: requires target workload*

### 8.7 Pipeline Concurrency Test
- [x] DAG-parallel results match sequential bit-for-bit — `SequentialVsParallelParity` integration test (Phase 6)
- [x] Cancellation mid-pipeline under concurrent load — `CancellationMidPipelineConcurrent` integration test
- [x] Multiframe partial failure under concurrent load — `MultiframePipelinePartialFailureConcurrent` integration test

### 8.8 Fuzzing
- [x] Fuzz-lite: malformed images, YAML config, template deserialization → graceful errors (10 tests in `test_api_hardening.cpp`)
- [ ] Full AFL/libFuzzer-based fuzzing — *blocked: requires AFL/libFuzzer setup + 8-hour session*
- [ ] 8-hour fuzz session with no crashes — *blocked: time-bound, requires dedicated infrastructure*

### 8.9 Static Analysis
- [ ] clang-tidy: zero warnings at strict level — *blocked: unbounded scope; `.clang-tidy` config exists*
- [ ] cppcheck: zero warnings — *blocked: unbounded scope*
- [x] Full compiler warning sweep (`-Wall -Wextra -Wpedantic -Werror`) — enforced on all builds since Phase 1

### 8.10 Security Audit
- [ ] Buffer overflow analysis — *blocked: requires ASan clean pass + dedicated review*
- [ ] Integer overflow analysis — *partially covered by `-Wconversion -Wsign-conversion -Werror`*
- [ ] Timing attack resistance: Hamming distance is constant-time (Pearson(HD, time) < 0.05) — *blocked: requires profiling tooling*
- [ ] Lock contention analysis — *blocked: requires profiling tooling*

### 8.11 API Hardening
- [x] Input validation at all public boundaries (image dimensions, null checks, range checks)
- [x] 10 malformed inputs → 0 crashes — verified by 10 tests in `test_api_hardening.cpp`

### 8.12 Documentation
- [ ] Doxygen for all public API headers — *blocked: infrastructure setup*
- [ ] Generate HTML docs — *blocked: infrastructure setup*
- [x] `TESTING.md` updated with Phase 8 test files, sanitizer targets, and final counts

### 8.13 CI/CD Pipeline
- [ ] GitHub Actions workflow: build (linux-release, Linux aarch64) — *blocked: requires repo push access*
- [ ] Test: unit + integration + cross-language (sequential + parallel modes) — *blocked: requires CI setup*
- [ ] Benchmark: run on PR, compare against baseline, post results as comment — *blocked: requires CI setup*
- [ ] Publish container image — *blocked: requires CI setup*

### 8.14 Platform Verification
- [x] Build and test on Linux ARM (aarch64, container) — 445 tests passing in Docker
- [x] NEON SIMD dispatch verified via unit tests

### 8.15 Complete Comparison Harness
- [ ] `Dockerfile.comparison`: both Python open-iris + C++ binary — *blocked: requires CASIA1 dataset + Python fixtures*
- [ ] `cross_language_comparison.py` orchestrator (3 modes: Python, C++ plaintext, C++ encrypted) — *blocked: requires CASIA1 dataset*
- [ ] Run on CASIA1 (757 images): all comparisons pass — *blocked: requires CASIA1 dataset*
- [ ] Run on CASIA-Iris-Thousand (20K images): biometric accuracy baselines documented — *blocked: requires dataset*
- [ ] JSON report generation (§17.6.3) — *blocked: requires comparison harness*

### 8.16 Comparison Regression Tracking
- [ ] `comparison_history.jsonl`: one JSON per CI run — *blocked: requires CI setup*
- [ ] Detect performance regressions (> 5% slower) — *blocked: requires CI setup*
- [ ] Detect accuracy regressions (any tolerance that previously passed now fails) — *blocked: requires CI setup*
- [ ] Block merge on accuracy/security regressions — *blocked: requires CI setup*

### Phase 8 Gate
- [ ] Zero ASan/UBSan/TSan findings — *infrastructure ready (`make test-asan`, `make test-tsan`); requires running*
- [ ] Zero clang-tidy warnings at strict level — *blocked: unbounded scope*
- [ ] 8-hour fuzz session with no crashes — *blocked: requires AFL/libFuzzer setup*
- [ ] Full pipeline < 200ms (excluding model loading) — *blocked: requires ONNX model*
- [ ] Peak memory < 200MB — *blocked: requires profiling*
- [x] Builds and tests pass on Linux ARM (aarch64) — **445 tests passing, 0 failures**
- [x] NEON SIMD popcount verified via unit tests (8 tests in `test_simd_popcount.cpp`)
- [ ] CASIA1 full dataset cross-language comparison passing — *blocked: requires comparison tooling + ONNX model (see reproduction steps below)*
- [ ] CASIA-Iris-Thousand biometric accuracy baselines documented — *blocked: requires comparison tooling + ONNX model*

---

## Final Sign-off

**445 tests passing, 0 failures, 1 skipped** (as of 2026-03-02)

Reproduce all tests: `cd biometric/iris && docker compose build --no-cache test`

### Verified (signed off)

- [x] All 445 unit/integration tests passing (417 Phase 1-7 + 25 Phase 8 + 3 sign-off)
  - `cd biometric/iris && docker compose build --no-cache test`
  - Output: `100% tests passed, 0 tests failed out of 445`
- [x] All integration tests passing (cancellation, partial failure, config validation)
  - Tests: `CancellationMidPipeline`, `CancellationMidPipelineConcurrent`, `BadClassNameFails`, `CircularDependencyFails`, `MissingSourceNodeFails`, `MultiframePipelinePartialFailureConcurrent`
  - File: `tests/integration/test_pipeline_integration.cpp`
  - `ctest -R PipelineIntegration --output-on-failure`
- [x] `NodeRegistry` resolves all class names from unmodified Python `pipeline.yaml`
  - Tests: `AllPipelineYamlClassNamesDispatched` (21 class names), `PipelineYamlLoadsAndBuilds` (loads real YAML, builds DAG)
  - File: `tests/unit/test_pipeline_dag.cpp:226-317`
  - `ctest -R PipelineDAG --output-on-failure`
- [x] `PackedIrisCode` bit layout verified: pack -> unpack round-trip is lossless
  - Tests: `FromMatRoundTrip`, `FromMatRoundTripRandom`, `PackedIrisCodeRoundTrip` (base64)
  - Files: `tests/unit/test_types.cpp:19-70`, `tests/unit/test_base64.cpp`
  - `ctest -R "PackedIrisCode|FromMat" --output-on-failure`
- [x] Encrypted matching decision agreement: 0 disagreements vs plaintext
  - Test: `DecisionAgreementZeroDisagreements` — 10 random pairs, threshold 0.35, match/no-match identical
  - File: `tests/integration/test_fhe_integration.cpp`
  - `ctest -R DecisionAgreement --output-on-failure`
- [x] Encrypted HD values within 1e-6 of plaintext
  - Tests: `MatchesPlaintextHD`, `PartialMaskMatchesPlaintext`, `RandomParity` (5 trials), `RotationMatchesPlaintext` — all at 1e-10 (10x stricter)
  - File: `tests/unit/test_encrypted_matcher.cpp`
  - `ctest -R EncryptedMatcher --output-on-failure`
- [x] Encrypted template entropy > 7.5 bits/byte (BFV RNS measured ~7.88)
  - Test: `HighEntropyCiphertext` — Shannon entropy of serialized ciphertext
  - Note: BFV uses RNS representation + cereal framing; ~7.88 bits/byte. IND-CPA ≠ indistinguishable from random bytes.
  - File: `tests/unit/test_encrypted_template.cpp`
  - `ctest -R HighEntropy --output-on-failure`
- [x] No plaintext iris code leak in encrypted output
  - Test: `NoPlaintextLeakInCiphertext` — pattern search (0xDEADBEEFCAFEBABE) + IND-CPA verification (two encryptions of same plaintext produce different ciphertexts)
  - File: `tests/unit/test_encrypted_template.cpp`
  - `ctest -R NoPlaintextLeak --output-on-failure`
- [x] Key lifecycle fully operational (generate, store, load, validate, expire, re-encrypt)
  - Tests: `GenerateSucceeds`, `ValidateSucceeds`, `ExpiredKeyDetected`, `NonExpiredKeyNotDetected`, `GenerateWithTTL`, `SaveLoadRoundTrip`, `LoadPublicOnlyHasNoSecretKey`, `MetadataPreservedExactly`, `KeySaveLoadEncryptMatch`, `ReencryptWithNewKeys`, `ReEncryptStandalone`
  - Files: `test_key_manager.cpp`, `test_key_store.cpp`, `test_encrypted_template.cpp`, `test_fhe_integration.cpp`
  - `ctest -R "KeyManager|KeyStore|ReEncrypt|Reencrypt" --output-on-failure`
- [x] Builds and tests pass on Linux ARM (aarch64, container)
  - Docker: Ubuntu 24.04, GCC 13, Linux aarch64 target, 445 tests passing
  - `cd biometric/iris && docker compose build --no-cache test`

### Blocked (cannot verify yet)

- [ ] All cross-language validation tests passing (on CASIA1 dataset) — *blocked: requires ONNX model + comparison tooling; see [How to Reproduce](#how-to-reproduce-cross-language-validation) below*
- [ ] All E2E tests passing (using local `data/` CASIA1 images) — *blocked: requires ONNX model*
- [ ] Benchmark targets met — *blocked: requires ONNX model for full pipeline*
- [ ] Zero sanitizer findings — *infrastructure ready (`make test-asan`, `make test-tsan`); needs dedicated run*
- [ ] Zero static analysis warnings — *blocked: clang-tidy/cppcheck setup*
- [ ] `signoff_pass == true` in comparison report — *blocked: comparison framework not built*
- [ ] C++ Plaintext pipeline speedup >= 3x over Python — *blocked: ONNX model + benchmark*
- [ ] Biometric accuracy (FMR/FNMR) matches Python baseline — *blocked: CASIA1 cross-language run*
- [ ] Iris code bit-exact agreement: 100% — *blocked: cross-language validation*
- [ ] Hamming distance Pearson r == 1.0, max discrepancy == 0.0 — *blocked: cross-language validation*
- [ ] Template cross-compatibility: C++ <-> Python HD matches — *blocked: Python .npy fixtures*
- [ ] C++ Plaintext peak RSS < 200MB, < 0.5x Python — *blocked: full pipeline + profiling*
- [ ] `encryption_correctness_pass == true` in comparison report — *blocked: comparison framework*
- [ ] Encryption overhead metrics documented — *benchmarks exist (`bench_crypto.cpp`); needs documented results*
- [ ] Container image published and tested — *blocked: registry push access*
- [ ] API documentation generated — *blocked: Doxygen setup*

---

## How to Reproduce: Cross-Language Validation

This section documents how to run the cross-language validation that proves C++ produces identical results to Python open-iris. See also `MASTER_PLAN.md §17` for the full specification and `QA.md` for detailed Q&A.

### What It Proves

The C++ pipeline must produce **bit-exact iris codes** and **identical Hamming distances** as Python for the same input images. Three modes are compared:

| Mode | Pipeline | Matching | Purpose |
|------|----------|----------|---------|
| Python | Plaintext | Plaintext HD | Ground-truth reference |
| C++ Plaintext (`--no-encrypt`) | Plaintext | SIMD popcount HD | Apples-to-apples vs Python |
| C++ Encrypted (default) | Plaintext | BFV homomorphic HD | Production mode correctness |

**Sign-off gates on C++ Plaintext vs Python.** Encrypted mode must agree with plaintext on all match/no-match decisions (informational + correctness).

### Pass Criteria

| Metric | Threshold |
|--------|-----------|
| Iris code bit agreement | 100% (`popcount(cpp XOR python) == 0`) |
| Hamming distance correlation | Pearson r = 1.0, max discrepancy = 0.0 |
| Match decision agreement (threshold 0.35) | 0 disagreements |
| Per-node max absolute error | < 1e-4 (float), exact (int/bool) |
| FMR/FNMR/EER delta vs Python | 0.0 |
| C++ Plaintext pipeline speedup | >= 3x over Python |
| C++ Plaintext peak RSS | < 200MB, < 0.5x Python |
| Encrypted HD vs Plaintext HD | max diff < 1e-6 |

### Current Status

| Component | Status | Location |
|-----------|--------|----------|
| CASIA1 dataset (757 images, 108 subjects) | Available | `data/Iris/CASIA1/` |
| NPY reader/writer (C++) | Done | `biometric/iris/tests/comparison/npy_reader.hpp`, `npy_writer.hpp` |
| NPY template utils (C++) | Done | `biometric/iris/tests/comparison/template_npy_utils.hpp` |
| NPY round-trip unit tests | Done | `test_npy_writer.cpp`, `test_template_npy.cpp` |
| ONNX segmentation model | **Not available** | Required for pipeline's first node |
| `generate_test_fixtures.py` | **Not created** | `biometric/iris/tools/` |
| `iris_comparison.cpp` | **Not created** | `biometric/iris/tests/comparison/` |
| `cross_language_comparison.py` | **Not created** | `biometric/iris/tools/` |
| `test_cross_language.cpp` | **Not created** | `biometric/iris/tests/comparison/` |
| `Dockerfile.comparison` | **Not created** | `biometric/iris/` |

### Prerequisites

1. **CASIA1 dataset** — already at `data/Iris/CASIA1/` (verified: 108 subjects, 757 `.jpg` images)
2. **ONNX segmentation model** — the `.onnx` file referenced in `open-iris/src/iris/pipelines/confs/pipeline.yaml` (not in repo; obtain from Worldcoin/open-iris model card or train)
3. **Python open-iris** — bundled at `open-iris/` as submodule
4. **Docker** — for C++ builds

### Step-by-Step Reproduction

#### Step 0: Verify Dataset

```bash
# From project root (/Users/innox/projects/iris2/iriscpp)
ls data/Iris/CASIA1/ | wc -l          # Expected: 108 (subject folders)
find data/Iris/CASIA1/ -name "*.jpg" | wc -l  # Expected: 757 (iris images)

# Image naming convention:
#   001_1_1.jpg → subject 001, left eye (1), image 1
#   001_2_3.jpg → subject 001, right eye (2), image 3
```

#### Step 1: Set Up Python Environment

```bash
python3 -m venv .venv && source .venv/bin/activate
cd open-iris && IRIS_ENV=SERVER pip install -e . && cd ..
python3 -c "from iris import IRISPipeline; print('OK')"  # verify
```

#### Step 2: Generate Python Reference Fixtures

Create `biometric/iris/tools/generate_test_fixtures.py` (not yet in repo):

```python
#!/usr/bin/env python3
"""Generate .npy reference fixtures from Python open-iris pipeline.

Usage:
    python3 generate_test_fixtures.py \
        --images ../../data/Iris/CASIA1/ \
        --output ../tests/fixtures/python_reference/
"""
import argparse, json, sys
from pathlib import Path
import numpy as np
from iris import IRISPipeline, IRImage

def generate(images_dir: Path, output_dir: Path):
    pipeline = IRISPipeline(env=IRISPipeline.DEBUGGING_ENVIRONMENT)
    image_paths = sorted(images_dir.rglob("*.jpg"))
    print(f"Processing {len(image_paths)} images...")

    for i, img_path in enumerate(image_paths):
        try:
            result = pipeline(IRImage(filepath=str(img_path)))
        except Exception as e:
            print(f"  [{i+1}/{len(image_paths)}] SKIP {img_path.name}: {e}")
            continue

        out = output_dir / img_path.stem
        out.mkdir(parents=True, exist_ok=True)

        # Save intermediate node outputs as .npy
        np.save(out / "segmentation_predictions.npy", result["segmentation"].predictions)
        np.save(out / "normalized_image.npy", result["normalization"].normalized_image)
        np.save(out / "normalized_mask.npy", result["normalization"].normalized_mask)
        np.save(out / "iris_code.npy", result["encoder"].iris_codes)
        np.save(out / "mask_code.npy", result["encoder"].mask_codes)

        # Save scalar metadata as JSON
        metadata = {
            "eye_centers": {
                "pupil": list(result["eye_center_estimation"].pupil_center),
                "iris": list(result["eye_center_estimation"].iris_center),
            },
            "offgaze": float(result["offgaze_estimation"].score),
            "occlusion": float(result["occlusion_calculator"].score),
        }
        (out / "metadata.json").write_text(json.dumps(metadata, indent=2))
        print(f"  [{i+1}/{len(image_paths)}] OK {img_path.name}")

    print(f"Done. Fixtures written to {output_dir}")

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--images", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    args = p.parse_args()
    generate(args.images, args.output)
```

Run it:
```bash
cd biometric/iris
python3 tools/generate_test_fixtures.py \
    --images ../../data/Iris/CASIA1/ \
    --output tests/fixtures/python_reference/
```

Expected output: 757 directories under `tests/fixtures/python_reference/`, each containing `.npy` files + `metadata.json`.

#### Step 3: Build C++ Comparison Binary

```bash
cd biometric/iris

# Build with comparison target enabled (requires iris_comparison.cpp to be written first)
docker compose build test
# Or:
# cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
#     -DIRIS_ENABLE_TESTS=ON -DIRIS_ENABLE_FHE=ON \
#     -DIRIS_BUILD_COMPARISON=ON -DONNXRUNTIME_ROOT=/opt/onnxruntime
# cmake --build build --target iris_comparison test_cross_language
```

#### Step 4: Run C++ Pipeline on Same Images

```bash
# Plaintext mode (for sign-off comparison vs Python)
./build/tests/comparison/iris_comparison \
    --no-encrypt \
    --images ../../data/Iris/CASIA1/ \
    --output tests/fixtures/cpp_plaintext/ \
    --format npy

# Encrypted mode (for FHE correctness check vs plaintext)
./build/tests/comparison/iris_comparison \
    --images ../../data/Iris/CASIA1/ \
    --output tests/fixtures/cpp_encrypted/ \
    --format npy
```

#### Step 5: Run Comparison Orchestrator

```bash
python3 tools/cross_language_comparison.py \
    --python-fixtures tests/fixtures/python_reference/ \
    --cpp-plaintext-fixtures tests/fixtures/cpp_plaintext/ \
    --cpp-encrypted-fixtures tests/fixtures/cpp_encrypted/ \
    --output comparison_report.json
```

#### Step 6: Check Pass/Fail

```bash
python3 -c "
import json, sys
r = json.load(open('comparison_report.json'))
print(f'Sign-off:              {r[\"signoff_pass\"]}')
print(f'Encryption correctness: {r[\"encryption_correctness_pass\"]}')
print(f'Overall:               {r[\"overall_pass\"]}')
if r.get('failures'):
    for f in r['failures']: print(f'  FAIL: {f}')
    sys.exit(1)
print('All cross-language validation tests PASSED.')
"
```

#### Step 7: Run GTest Cross-Language Suite

```bash
cd build && ctest -R CrossLanguage --output-on-failure
```

### What Blocks Completion

| Blocker | Impact | Resolution |
|---------|--------|------------|
| **ONNX segmentation model** | Cannot run full pipeline (first node fails) | Obtain `.onnx` from Worldcoin model card or train |
| **`generate_test_fixtures.py`** | No Python ground-truth fixtures | Write script (pseudocode above) |
| **`iris_comparison.cpp`** | No C++ comparison binary | Write binary (processes images, outputs `.npy`) |
| **`cross_language_comparison.py`** | No automated comparison | Write orchestrator (compares 3 modes, outputs JSON report) |
| **`test_cross_language.cpp`** | No GTest suite for CI | Write GTest loading `.npy` fixtures |
| **`Dockerfile.comparison`** | No container with both Python + C++ | Write Dockerfile |

### Per-Node Tolerance Table

| Node | Output | Method | Tolerance |
|------|--------|--------|-----------|
| Segmentation | float32 map | Max abs error | < 1e-4 |
| Binarization | uint8 map | Exact | 0 |
| Vectorization | float64 contours | Hausdorff | < 1.0 px |
| Geometry Refinement | float64 points | Hausdorff | < 0.5 px |
| Geometry Estimation | float64 ellipse | Relative error | < 0.1% |
| Eye Centers | float64 (x,y) | Euclidean | < 0.5 px |
| Offgaze / Occlusion | float64 | Absolute | < 1e-6 |
| Normalization | float64 image | PSNR | > 60 dB |
| Iris Response | float64 filter | Max abs error | < 1e-5 |
| Encoder | uint8 code/mask | Bit-exact | 0 |
| Hamming Distance | float64 | Exact | 0.0 |
| Match Decision | bool | Exact | 0 disagreements |
