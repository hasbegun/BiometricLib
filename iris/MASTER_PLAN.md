# IRIS C++ Master Plan

## Conversion of Worldcoin open-iris (Python) to C++23 with OpenFHE Integration

---

## Table of Contents

1. [Project Goals](#1-project-goals)
2. [Source Project Analysis](#2-source-project-analysis)
3. [Target Architecture](#3-target-architecture)
4. [Technology Stack](#4-technology-stack)
5. [Module Conversion Plan](#5-module-conversion-plan)
6. [OpenFHE Integration Plan](#6-openfhe-integration-plan)
7. [Container Strategy](#7-container-strategy)
8. [Build System](#8-build-system)
9. [Implementation Phases](#9-implementation-phases)
10. [Test Plan](#10-test-plan)
11. [Verification Plan](#11-verification-plan)
12. [Risk Assessment](#12-risk-assessment)
13. [Error Recovery & Resilience](#13-error-recovery--resilience)
14. [Platform & Cross-Compilation](#14-platform--cross-compilation)
15. [YAML Config Compatibility & Node Registry](#15-yaml-config-compatibility--node-registry)
16. [Iris Code Bit-Packing Layout](#16-iris-code-bit-packing-layout)
17. [Cross-Language Comparison Framework](#17-cross-language-comparison-framework-c-vs-python)

---

## Implementation Progress

### Overall Status

| Phase | Status | Tests | Description |
|-------|--------|-------|-------------|
| **Phase 1** | **DONE** | 50 | Foundation: build system, core types, thread pool, async I/O, encoder, SIMD popcount, node registry |
| **Phase 2** | **DONE** | 56 | Image processing & geometry: all 13 algorithm nodes from segmentation through geometry estimation |
| Phase 3 | Not started | — | Eye properties & normalization |
| Phase 4 | Not started | — | Feature extraction & encoding |
| Phase 5 | Not started | — | Matching & template operations |
| Phase 6 | Not started | — | Pipeline orchestration |
| Phase 7 | Not started | — | OpenFHE integration |
| Phase 8 | Not started | — | Hardening & optimization |

**Current total: 106 tests passing, 0 failures** (`docker compose build test`)

### Phase 1 — Completed Deliverables

| Task | Node / Component | Registration Name | Test File | Tests |
|------|------------------|-------------------|-----------|-------|
| 1.1 | Project scaffolding | — | — | — |
| 1.2 | Dockerfile (multi-stage) | — | — | — |
| 1.3 | Core types (`types.hpp`) | — | `test_types.cpp` | 9 |
| 1.4 | Error handling (`error.hpp`) | — | `test_error.cpp` | 4 |
| 1.5 | Algorithm CRTP base | — | — | — |
| 1.6 | Thread pool | — | `test_thread_pool.cpp` | 12 |
| 1.7 | Async I/O | — | `test_async_io.cpp` | 5 |
| 1.8 | Config parser + Node registry | — | `test_node_registry.cpp` | 4 |
| 1.9 | IrisEncoder | `iris.IrisEncoder` | `test_encoder.cpp` | 4 |
| 1.10 | SIMD popcount | — | `test_simd_popcount.cpp` | 8 |
| 1.11 | CancellationToken | — | `test_cancellation.cpp` | 4 |

### Phase 2 — Completed Deliverables

| Step | Node / Component | Registration Name | Test File | Tests |
|------|------------------|-------------------|-----------|-------|
| 1 | `geometry_utils` (shared) | — | `test_geometry_utils.cpp` | 15 |
| 2 | `SpecularReflectionDetector` | `iris.SpecularReflectionDetection` | `test_specular_reflection.cpp` | 4 |
| 3 | `SegmentationBinarizer` | `iris.MultilabelSegmentationBinarization` | `test_binarization.cpp` | 5 |
| 4 | `ContourExtractor` | `iris.ContouringAlgorithm` | `test_vectorization.cpp` | 4 |
| 5 | `ContourInterpolator` | `iris.ContourInterpolation` | `test_geometry_refinement.cpp` | 3 |
| 6 | `EyeballDistanceFilter` | `iris.ContourPointNoiseEyeballDistanceFilter` | `test_geometry_refinement.cpp` | 2 |
| 7 | `BisectorsEyeCenter` | `iris.BisectorsMethod` | `test_eye_centers.cpp` | 5 |
| 8 | `MomentOrientation` | `iris.MomentOfArea` | `test_eye_orientation.cpp` | 5 |
| 9 | `ContourSmoother` | `iris.Smoothing` | `test_geometry_estimation.cpp` | 3 |
| 10 | `LinearExtrapolator` | `iris.LinearExtrapolation` | `test_geometry_estimation.cpp` | 4 |
| 11 | `EllipseFitter` | `iris.LSQEllipseFitWithRefinement` | `test_geometry_estimation.cpp` | 3 |
| 12 | `FusionExtrapolator` | `iris.FusionExtrapolation` | `test_geometry_estimation.cpp` | 1 |
| 13 | `OnnxSegmentation` | `iris.MultilabelSegmentation.create_from_hugging_face` | `test_segmentation.cpp` | 2 |

### Test Inventory Summary

| Test File | Suite(s) | Count | What is Tested |
|-----------|----------|-------|----------------|
| `test_types.cpp` | PackedIrisCode, IRImage, IrisTemplate, DistanceMatrix | 9 | Pack/unpack round-trip, rotation, construction, symmetry |
| `test_error.cpp` | ErrorCode, MakeError, Result | 4 | Error code names, unexpected creation, value/error holding |
| `test_cancellation.cpp` | CancellationToken | 4 | Init state, cancel flag, double-cancel safety, concurrent access |
| `test_thread_pool.cpp` | ThreadPool, GlobalPool | 12 | Submit/wait, parallel_for, batch, exceptions, counts, shutdown |
| `test_async_io.cpp` | AsyncIOTest | 5 | File write/read, nonexistent file, concurrent reads, YAML, download |
| `test_simd_popcount.cpp` | SimdPopcount | 8 | Zeros, ones, identical, known patterns, scalar parity, small, AND, XOR-AND |
| `test_encoder.cpp` | IrisEncoder | 4 | Binarization, mask threshold, empty input, NodeParams construction |
| `test_node_registry.cpp` | NodeRegistry | 4 | Unknown lookup, register/lookup, auto-registration, registered names |
| `test_geometry_utils.cpp` | Cartesian2Polar, Polar2Cartesian, PolygonArea, EstimateDiameter, FilledMask | 15 | Polar conversions, round-trips, contour overload, area, diameter, mask filling |
| `test_specular_reflection.cpp` | SpecularReflection | 4 | Basic threshold, custom threshold, empty image, NodeParams |
| `test_binarization.cpp` | SegmentationBinarizer | 5 | Default thresholds, below threshold, mixed values, custom thresholds, empty input |
| `test_vectorization.cpp` | ContourExtractor | 4 | Circular contours, empty iris, area filter, circle approximation |
| `test_geometry_refinement.cpp` | ContourInterpolator, EyeballDistanceFilter | 5 | Large gaps, dense (no-op), empty iris, noise removal, no-noise passthrough |
| `test_eye_centers.cpp` | BisectorsEyeCenter | 5 | Perfect circle, off-center pupil, partial arc, too few points, custom params |
| `test_eye_orientation.cpp` | MomentOrientation | 5 | Horizontal/vertical/rotated ellipse, circular rejection, too few points |
| `test_geometry_estimation.cpp` | ContourSmoother, LinearExtrapolator, EllipseFitter, FusionExtrapolator | 11 | Smoothing shape/points, extrapolation/radius/dphi, ellipse fit/reject/few-pts, fusion |
| `test_segmentation.cpp` | OnnxSegmentation | 2 | Invalid model path throws, invalid NodeParams throws |
| **Total** | | **106** | |

### How to Verify

```bash
# Full build + test (the single source of truth)
cd biometric/iris && docker compose build test

# Expected output (last lines):
# 100% tests passed, 0 tests failed out of 106
# Total Test time (real) = 0.25 sec
```

All tests use **synthetic data** (programmatically generated circles, ellipses, masks) — no external model files or images required. The OnnxSegmentation tests verify error paths (invalid model path) since the ONNX model file is not bundled in the test fixtures.

Cross-language comparison tests against Python reference data are planned for after Phase 2 (see §17 and the Phase 2 plan addendum).

### Pending (Not Yet Started)

- **Cross-language comparison fixtures**: `tools/generate_test_fixtures.py` to run Python pipeline and save intermediate `.npy` outputs
- **Cross-language comparison tests**: `tests/comparison/test_phase2_cross_language.cpp` to load `.npy` fixtures and compare C++ output against Python
- **Phase 3+**: eye properties, normalization, feature extraction, matching, pipeline orchestration, OpenFHE, hardening

---

## 1. Project Goals

| Goal | Description |
|------|-------------|
| **Performance** | Achieve 5-10x speedup over Python for image processing and template operations |
| **Memory Efficiency** | Reduce peak memory usage by eliminating Python object overhead and GC pressure |
| **Tamper Resistance** | Compiled binary with no exposed source; encrypted templates via OpenFHE |
| **Security First** | Homomorphic encryption for iris template storage and matching from day one |
| **Container Isolation** | All builds and runtime in containers; zero host pollution |
| **Feature Parity** | 100% of open-iris pipeline functionality reproduced in C++ |
| **C++23** | Modern C++ with concepts, ranges, std::expected, std::format, structured bindings |

---

## 2. Source Project Analysis

### 2.1 open-iris Overview

- **Origin**: Worldcoin Foundation (now "World")
- **Purpose**: Iris recognition for biometric uniqueness verification at billion-user scale
- **License**: MIT
- **Version**: 1.11.0
- **Python**: 3.8-3.10
- **Codebase**: ~85 Python files, ~10,600 lines of code

### 2.2 Complete Feature Inventory

The pipeline processes an infrared eye image through a 24-node DAG to produce a binary iris code:

```
IR Image Input
    |
    v
[1] Segmentation (UNet++/MobileNetV2 ONNX model)
    |
    v
[2] Segmentation Binarization --> [3] Specular Reflection Detection
    |                                       |
    v                                       |
[4] Vectorization (contour extraction)      |
    |                                       |
    v                                       |
[5] Contour Interpolation                   |
    |                                       |
    v                                       |
[6] Distance Filter (eyeball noise)         |
    |                                       |
    +--------+--------+                     |
    |        |        |                     |
    v        v        v                     |
[7] Eye   [8] Eye  [9] Smoothing            |
 Orient.   Centers                          |
    |        |        |                     |
    |        v        |                     |
    |   [10] Centers  |                     |
    |    Validator    |                     |
    |        |        |                     |
    +--------+--------+                     |
             |                              |
             v                              |
      [11] Geometry Estimation              |
       (Fusion: Linear + LSQ Ellipse)       |
             |                              |
    +--------+--------+--------+            |
    |        |        |        |            |
    v        v        v        v            |
[12]Pupil [13]Off  [14]Occ   [15]Occ        |
 /Iris    gaze     90deg     30deg          |
 Property                                   |
             |                              |
             v                              v
      [16] Noise Mask Aggregation <---------+
             |
             v
      [17] Normalization (rubber-sheet model)
             |
             +--------+
             |        |
             v        v
      [18] Sharp  [19] Filter Bank
       ness        (2x Gabor filters)
                      |
                      v
               [20] Fragile Bit Refinement
                      |
                      v
               [21] Iris Encoder
                      |
                      v
               [22] Mask Size Validator
                      |
                      v
               [23] Bounding Box Calculator
                      |
                      v
                  IrisTemplate Output
```

### 2.3 Feature Breakdown by Module

#### Segmentation (`nodes/segmentation/`)
- ONNX Runtime inference with UNet++/MobileNetV2
- Input: IR image (grayscale) -> tensor Nx3x640x480
- Output: 4-class segmentation map (eyeball, iris, pupil, eyelashes)
- Model download from HuggingFace Hub
- Optional denoising

#### Binarization (`nodes/binarization/`)
- `MultilabelSegmentationBinarization`: converts segmentation map to binary masks per class
- `SpecularReflectionDetection`: identifies specular reflections as noise

#### Vectorization (`nodes/vectorization/`)
- `ContouringAlgorithm`: extracts polygon contours from binary masks using OpenCV `findContours`
- Area-based filtering to select primary contour per class

#### Geometry Refinement (`nodes/geometry_refinement/`)
- `ContourInterpolation`: interpolates contour points for uniform spacing
- `ContourPointNoiseEyeballDistanceFilter`: removes noisy points based on distance to eyeball boundary
- `Smoothing`: rolling median filter in polar coordinates

#### Geometry Estimation (`nodes/geometry_estimation/`)
- `LinearExtrapolation`: polar-space linear extrapolation of iris/pupil boundaries
- `LSQEllipseFitWithRefinement`: least-squares ellipse fitting with iterative refinement
- `FusionExtrapolation`: combines both strategies, switches based on shape statistics (std threshold 0.014)

#### Eye Properties Estimation (`nodes/eye_properties_estimation/`)
- `BisectorsMethod`: eye center via perpendicular bisector intersection (100 bisectors default)
- `MomentOfArea`: eye orientation via second-order moments
- `EccentricityOffgazeEstimation`: off-gaze from contour eccentricity
- `OcclusionCalculator`: fraction of iris visible at configurable quantile angle (30, 90 degrees)
- `PupilIrisPropertyCalculator`: diameter ratio + center distance ratio
- `SharpnessEstimation`: Laplacian variance of normalized image
- `IrisBBoxCalculator`: bounding box around iris region

#### Normalization (`nodes/normalization/`)
- `LinearNormalization`: Daugman rubber-sheet model (Cartesian -> polar)
- `NonlinearNormalization`: nonlinear squared transformation with optional Wyatt correction
- `PerspectiveNormalization`: region-based perspective transform (trapezoid -> rectangle)
- Default output: 128 radial x 1024 angular samples

#### Iris Response (`nodes/iris_response/`)
- `ConvFilterBank`: applies multiple filters with probe schemas
- `GaborFilter`: 2D spatial Gabor wavelet with DC correction and fixed-point option
  - Parameters: kernel_size, sigma_phi, sigma_rho, theta_degrees, lambda_phi
- `LogGaborFilter`: frequency-domain log-Gabor via 2D FFT
- `RegularProbeSchema`: uniform grid sampling (16 rows x 256 columns default)

#### Iris Response Refinement (`nodes/iris_response_refinement/`)
- `FragileBitRefinement`: identifies unstable bits near decision boundary
  - Supports polar and cartesian fragile type
  - Configurable value thresholds per filter

#### Encoder (`nodes/encoder/`)
- `IrisEncoder`: binarizes complex filter response to 2 bits per position
  - Real component sign -> bit 0
  - Imaginary component sign -> bit 1
  - Output: iris codes of shape (16, 256, 2) per filter

#### Matcher (`nodes/matcher/`)
- `HammingDistanceMatcher`: full-featured matching
  - Rotation shift tolerance: +/-15 positions
  - Normalized Hamming distance correction for mask size
  - Separate upper/lower half matching
  - Per-bit weights support
- `SimpleHammingDistanceMatcher`: basic HD computation
- `BatchMatcher`: gallery-to-gallery and intra-gallery batch operations

#### Template Operations (`nodes/templates_*`)
- `HammingDistanceBasedAlignment`: aligns templates via rotation optimization
- `MajorityVoteAggregation`: combines multiple templates through bit voting
- `TemplateIdentityFilter`: validates template consistency

#### Validators (`nodes/validators/`)
- `IsMaskTooSmallValidator`: minimum unmasked bits (4096)
- `IsPupilInsideIrisValidator`: anatomical correctness check
- `OcclusionValidator`: minimum visibility threshold (25-30%)
- `OffgazeValidator`: maximum off-axis angle (0.8)
- `PolygonsLengthValidator`: minimum contour completeness
- `Pupil2IrisPropertyValidator`: diameter ratio bounds (0.1-0.7)
- `SharpnessValidator`: minimum Laplacian variance (461.0)
- `ExtrapolatedPolygonsInsideImageValidator`: boundary check
- `EyeCentersInsideImageValidator`: centers within image bounds

#### Orchestration (`orchestration/`)
- DAG-based pipeline execution from YAML configuration
- `Environment`: controls error handling, QA gating, output building
- Error management strategies: raise vs store
- Output builders: simple, debugging, serialized

#### Pipelines (`pipelines/`)
- `IRISPipeline`: single-image iris code extraction
- `MultiframeIrisPipeline`: multi-image processing with template aggregation
- `TemplatesAggregationPipeline`: template fusion from multiple captures

### 2.4 Data Types (Python -> C++ mapping planned in Section 5)

| Python Type | Description | Shape/Size |
|-------------|-------------|------------|
| `IRImage` | Input infrared image | HxW uint8 |
| `SegmentationMap` | Per-pixel class predictions | 4xHxW float32 |
| `GeometryMask` | Binary masks for iris/pupil/eyeball | 3 x HxW bool |
| `NoiseMask` | Combined noise mask | HxW bool |
| `GeometryPolygons` | Pupil/iris/eyeball contours | Nx2 float32 each |
| `EyeCenters` | Pupil/iris center points | 2 x (x,y) float64 |
| `EyeOrientation` | Orientation angle | float |
| `NormalizedIris` | Unwrapped iris image + mask | 128x1024 float64 |
| `IrisFilterResponse` | Complex filter outputs | list of (16,256) complex |
| `IrisTemplate` | Final iris code + mask | list of (16,256,2) bool |
| `IrisTemplateWithId` | Template + source image ID | IrisTemplate + string |
| `WeightedIrisTemplate` | Template + per-bit reliability weights | IrisTemplate + list of float arrays |
| `AlignedTemplates` | Aligned templates + distance matrix + reference ID | aggregation output |
| `Landmarks` | Pupil/iris/eyeball landmark points | 3 x Nx2 float32 |
| `DistanceMatrix` | Pairwise Hamming distances between templates | Dict[(i,j) -> float] |
| `Offgaze` | Off-gaze score | float |
| `Sharpness` | Sharpness score | float |
| `EyeOcclusion` | Occlusion fraction | float |
| `PupilToIrisProperty` | Diameter/center ratios | 2 floats |
| `BoundingBox` | Iris bounding rectangle | 4 floats |

---

## 3. Target Architecture

### 3.1 Project Structure

```
iriscpp/
├── MASTER_PLAN.md                  # This file
├── todo.md                         # Project goals/notes
├── open-iris/                      # Source Python project (reference)
├── data -> ../eyed/data/Iris/CASIA1  # Symlink to local CASIA1 test dataset
│
└── biometric/iris/                 # *** All C++ output lives here ***
    ├── CMakeLists.txt              # Root build file
    ├── Dockerfile                  # Multi-stage container build
    ├── docker-compose.yml          # Dev/test/prod services
    ├── .dockerignore
    ├── .clang-format
    ├── .clang-tidy
    ├── conan/                      # Conan package manager profiles
    │   └── conanfile.py
    ├── cmake/
    │   ├── FindOpenFHE.cmake
    │   ├── FindONNXRuntime.cmake
    │   └── CompilerWarnings.cmake
    ├── CMakePresets.json            # Build presets (linux-release, linux-aarch64, sanitizers)
    ├── include/iris/               # Public headers
    │   ├── iris.hpp                # Main include (facade)
    │   ├── core/
    │   │   ├── types.hpp           # All data types (IRImage, IrisTemplate, etc.)
    │   │   ├── iris_code_packed.hpp # Bit-packed iris code for SIMD matching (§16)
    │   │   ├── algorithm.hpp       # Algorithm base class (CRTP)
    │   │   ├── node_registry.hpp   # Python class_name → C++ factory map (§15)
    │   │   ├── pipeline_node.hpp   # Pipeline DAG node
    │   │   ├── pipeline_executor.hpp # DAG-parallel pipeline executor
    │   │   ├── thread_pool.hpp     # Work-stealing thread pool
    │   │   ├── async_io.hpp        # Async file/network I/O with retry (§13.1.3)
    │   │   ├── cancellation.hpp    # Cooperative cancellation token (§13.2)
    │   │   ├── environment.hpp     # Pipeline environment config
    │   │   ├── error.hpp           # Error types (std::expected based)
    │   │   └── config.hpp          # YAML config parsing
    │   ├── nodes/
    │   │   ├── segmentation.hpp
    │   │   ├── binarization.hpp
    │   │   ├── vectorization.hpp
    │   │   ├── geometry_refinement.hpp
    │   │   ├── geometry_estimation.hpp
    │   │   ├── eye_properties.hpp
    │   │   ├── normalization.hpp
    │   │   ├── iris_response.hpp
    │   │   ├── encoder.hpp
    │   │   ├── matcher.hpp
    │   │   ├── matcher_simd.hpp      # NEON SIMD popcount (§14.2)
    │   │   ├── template_ops.hpp
    │   │   └── validators.hpp
    │   ├── pipeline/
    │   │   ├── iris_pipeline.hpp
    │   │   ├── multiframe_pipeline.hpp
    │   │   └── aggregation_pipeline.hpp
    │   ├── crypto/
    │   │   ├── fhe_context.hpp        # OpenFHE wrapper
    │   │   ├── encrypted_template.hpp # Encrypted iris template
    │   │   ├── encrypted_matcher.hpp  # Matching under encryption
    │   │   ├── key_manager.hpp        # KeyManager + KeyBundle + KeyMetadata (§6.6.1)
    │   │   ├── key_store.hpp          # Directory-based key persistence (§6.6.2)
    │   │   └── secure_buffer.hpp      # RAII zero-on-destroy buffer (§6.6.3)
    │   └── utils/
    │       ├── math.hpp
    │       ├── image.hpp
    │       ├── base64.hpp              # Base64 encode/decode (§3.5)
    │       └── template_serializer.hpp # Binary + Python-compatible serialization (§3.5)
    ├── src/                        # Implementation files
    │   ├── core/
    │   ├── nodes/
    │   ├── pipeline/
    │   ├── crypto/
    │   └── utils/
    ├── tests/
    │   ├── unit/                   # Per-module unit tests
    │   │   ├── test_types.cpp
    │   │   ├── test_thread_pool.cpp
    │   │   ├── test_async_io.cpp
    │   │   ├── test_segmentation.cpp
    │   │   ├── test_binarization.cpp
    │   │   ├── test_vectorization.cpp
    │   │   ├── test_geometry_refinement.cpp
    │   │   ├── test_geometry_estimation.cpp
    │   │   ├── test_eye_properties.cpp
    │   │   ├── test_normalization.cpp
    │   │   ├── test_iris_response.cpp
    │   │   ├── test_encoder.cpp
    │   │   ├── test_matcher.cpp
    │   │   ├── test_template_ops.cpp
    │   │   ├── test_validators.cpp
    │   │   ├── test_pipeline.cpp
    │   │   ├── test_crypto.cpp
    │   │   └── test_encrypted_matcher.cpp
    │   ├── integration/            # Cross-module integration tests
    │   │   ├── test_full_pipeline.cpp
    │   │   └── test_encrypted_pipeline.cpp
    │   ├── e2e/                    # End-to-end with real images
    │   │   └── test_e2e_pipeline.cpp
    │   ├── bench/                  # Performance benchmarks
    │   │   ├── bench_pipeline.cpp
    │   │   ├── bench_matcher.cpp
    │   │   └── bench_crypto.cpp
    │   ├── comparison/             # Cross-language comparison tests (see §17)
    │   │   ├── CMakeLists.txt
    │   │   ├── iris_comparison.cpp     # C++ comparison binary
    │   │   ├── test_cross_language.cpp # GTest cross-language assertions
    │   │   └── npy_reader.hpp          # .npy file reader for C++
    │   └── fixtures/               # Test data
    │       ├── sample_ir_image.bin
    │       ├── expected_template.bin
    │       └── python_reference/   # Reference outputs from Python (.npy format)
    ├── tools/
    │   ├── generate_test_fixtures.py      # Python script to produce reference data
    │   ├── compare_outputs.py             # Cross-language comparison tool
    │   ├── cross_language_comparison.py   # Orchestrator: runs both C++ & Python, compares all metrics (§17.6)
    │   └── bench_python_pipeline.py       # Python benchmarker for performance baselines
    ├── Dockerfile.comparison              # Container with both Python open-iris + C++ binary (§17.6)
    └── models/
        └── iris_segmentation.onnx  # Segmentation model file
```

### 3.2 Design Principles

1. **Zero-cost abstractions**: Use CRTP for static polymorphism on algorithm nodes (no vtable overhead in the hot path)
2. **Value semantics**: Data types are value types with move semantics; no raw pointers
3. **Error handling**: `std::expected<T, IrisError>` throughout (no exceptions in hot paths)
4. **Memory**: Pre-allocated buffers with `std::pmr` for pipeline scratch memory; arena allocators for batch operations
5. **SIMD**: Use OpenCV's SIMD-optimized routines + manual SIMD for Hamming distance (popcount intrinsics)
6. **Compile-time**: `constexpr` filter kernels where parameters are known; `consteval` for Gabor kernel generation
7. **Async-first I/O**: All disk I/O (model loading, config, template serialization) and network I/O (model download) are async by default
8. **Parallel pipeline**: DAG-aware parallel execution of independent pipeline nodes; thread pool for compute-heavy stages
9. **Concurrent multiframe**: Multiple images processed in parallel with a shared thread pool
10. **Configuration**: YAML pipeline configs compatible with Python version (same structure, same parameters)

### 3.3 Concurrency & Async Architecture

The Python open-iris executes everything **strictly sequentially** -- single-threaded, synchronous I/O, no parallelism. This is one of the biggest performance wins for the C++ port. Every identified bottleneck gets a concurrency strategy:

#### 3.3.1 Thread Pool (`iris::ThreadPool`)

A shared work-stealing thread pool (based on `std::jthread`) sized to hardware concurrency. All parallel work is submitted to this pool -- no ad-hoc thread creation.

```cpp
// biometric/iris/include/iris/core/thread_pool.hpp
namespace iris {

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    // Submit a task, get a future
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;

    // Submit multiple tasks, wait for all
    template<typename Iter>
    void parallel_for(Iter begin, Iter end, auto&& fn);

    // Batch submit, return vector of futures
    template<typename F>
    auto batch_submit(size_t count, F&& factory) -> std::vector<std::future<void>>;

    size_t thread_count() const noexcept;
};

// Global pool accessor (initialized once, used everywhere)
ThreadPool& global_pool();

} // namespace iris
```

#### 3.3.2 Async I/O (`iris::AsyncIO`)

All disk and network I/O runs on a dedicated I/O thread pool (2-4 threads) to never block compute threads.

```cpp
// biometric/iris/include/iris/core/async_io.hpp
namespace iris {

class AsyncIO {
public:
    // Async file read -- returns future with file contents
    static std::future<std::vector<uint8_t>> read_file(const std::filesystem::path& path);

    // Async file write
    static std::future<void> write_file(const std::filesystem::path& path,
                                         std::span<const uint8_t> data);

    // Async model download (HTTP GET with caching)
    static std::future<std::filesystem::path> download_model(
        const std::string& url,
        const std::filesystem::path& cache_dir);

    // Async YAML config load
    static std::future<YAML::Node> load_yaml(const std::filesystem::path& path);
};

} // namespace iris
```

#### 3.3.3 Parallel Pipeline DAG Execution

The Python pipeline runs 24 nodes in a flat sequential loop. The C++ version analyzes the DAG and executes independent nodes concurrently:

```
DAG Parallelism Map (from pipeline.yaml):

Level 0: [segmentation] [specular_reflection_detection]     <-- PARALLEL
Level 1: [segmentation_binarization]
Level 2: [vectorization]
Level 3: [interpolation]
Level 4: [distance_filter]
Level 5: [eye_orientation] [eye_center_estimation]           <-- PARALLEL
Level 6: [eye_centers_inside_image_validator]
Level 7: [smoothing]
Level 8: [geometry_estimation]
Level 9: [pupil_to_iris_property] [offgaze] [occ90] [occ30] <-- PARALLEL (4-way)
Level 10: [noise_masks_aggregation]
Level 11: [normalization]
Level 12: [sharpness_estimation] [filter_bank]               <-- PARALLEL
Level 13: [iris_response_refinement]
Level 14: [encoder]
Level 15: [bounding_box_estimation]
```

Implementation: topological sort of the DAG, then execute each level's independent nodes concurrently via the thread pool.

```cpp
// biometric/iris/include/iris/core/pipeline_executor.hpp
namespace iris {

class PipelineExecutor {
public:
    explicit PipelineExecutor(ThreadPool& pool);

    // Build execution schedule from DAG
    struct ExecutionLevel {
        std::vector<std::string> node_names;  // nodes in this level run in parallel
    };
    std::vector<ExecutionLevel> schedule(const PipelineDAG& dag) const;

    // Execute the full pipeline with DAG-level parallelism
    Result<PipelineOutput> execute(
        const std::vector<ExecutionLevel>& levels,
        const NodeRegistry& nodes,
        PipelineContext& ctx) const;
};

} // namespace iris
```

#### 3.3.4 Bottleneck-Specific Concurrency Strategies

| Bottleneck | Python Behavior | C++ Strategy |
|-----------|----------------|--------------|
| **ConvFilterBank** (nested 16x256 loop per filter) | Sequential Python for-loop, 8192 iterations | `cv::filter2D` (OpenCV's internal SIMD/threading) + `parallel_for` over independent filters |
| **Normalization** (per-pixel list comprehension) | Python list comp, ~65K iterations | `cv::remap` with precomputed maps (single vectorized call, OpenCV internally parallelized) |
| **PerspectiveNormalization** (triple-nested loop) | Python per-pixel interpolation | `cv::warpPerspective` per region (OpenCV SIMD) + `parallel_for` over angle segments |
| **ONNX Segmentation** (model inference) | Single-threaded CPUExecutionProvider | ONNX Runtime with `ORT_PARALLEL` execution mode + intra-op thread count set to hardware cores |
| **Bilateral filter** (denoising) | `cv::bilateralFilter` (already C but single call) | OpenCV with `setNumThreads()` for internal parallelism |
| **Hamming distance rotation** (31 shifts per pair) | Python loop, 31 iterations with numpy ops | SIMD popcount + `parallel_for` over rotations for batch matching |
| **Pairwise template alignment** (O(N^2) pairs) | Sequential nested Python loops | `parallel_for` over independent pairs on thread pool |
| **Multiframe images** (N images sequential) | Sequential `for img in images` | `parallel_for` over all images, aggregate after barrier |
| **Batch matching** (1-vs-N gallery) | Sequential loop | `parallel_for` over gallery entries; SIMD inner loop |
| **Model loading** (ONNX load + validate) | Synchronous, model loaded twice | `AsyncIO::read_file` + load once into `OrtSession` directly from memory buffer |
| **Model download** (HuggingFace) | Synchronous HTTP with fast-transfer disabled | `AsyncIO::download_model` with libcurl async + local cache check first |
| **YAML config loading** | Synchronous `open()` + `yaml.safe_load()` | `AsyncIO::load_yaml` (non-blocking) |
| **Template serialization** (base64 encode/write) | Synchronous numpy + base64 | `AsyncIO::write_file` for disk; bit-pack with SIMD in compute thread |
| **Pipeline intermediate storage** | All intermediates held in Python dict | Move semantics + `std::optional` for lazy evaluation; only retain what downstream nodes need |

#### 3.3.5 Multiframe Parallel Processing

```cpp
// biometric/iris/include/iris/pipeline/multiframe_pipeline.hpp
namespace iris {

class MultiframePipeline {
public:
    // Process N images in parallel, then aggregate
    Result<IrisTemplate> run(std::span<const IRImage> images) {
        // Phase 1: parallel per-image pipeline (embarrassingly parallel)
        std::vector<std::future<Result<PipelineOutput>>> futures;
        futures.reserve(images.size());
        for (const auto& img : images) {
            futures.push_back(pool_.submit([&, this] {
                return iris_pipeline_.run(img);
            }));
        }

        // Phase 2: collect results (barrier)
        std::vector<IrisTemplate> templates;
        for (auto& fut : futures) {
            if (auto result = fut.get(); result.has_value()) {
                templates.push_back(std::move(result->iris_template.value()));
            }
        }

        // Phase 3: parallel pairwise alignment
        auto aligned = aligner_.align_parallel(templates, pool_);

        // Phase 4: aggregate via majority vote
        return aggregator_.aggregate(aligned);
    }
private:
    ThreadPool& pool_;
    IrisPipeline iris_pipeline_;
    TemplateAligner aligner_;
    MajorityVoteAggregator aggregator_;
};

} // namespace iris
```

#### 3.3.6 Parallel Batch Matching

```cpp
// biometric/iris/include/iris/nodes/matcher.hpp (partial)
namespace iris {

class BatchMatcher {
public:
    // One probe vs N gallery -- parallelized
    std::vector<double> match_one_vs_many(
        const IrisTemplate& probe,
        std::span<const IrisTemplate> gallery) const
    {
        std::vector<double> distances(gallery.size());
        pool_.parallel_for(0u, gallery.size(), [&](size_t i) {
            distances[i] = matcher_.run(probe, gallery[i]);
        });
        return distances;
    }

    // N-vs-N pairwise -- upper triangle parallelized
    DistanceMatrix match_pairwise(
        std::span<const IrisTemplate> templates) const
    {
        const size_t n = templates.size();
        DistanceMatrix matrix(n);
        // Flatten upper triangle indices for load balancing
        std::vector<std::pair<size_t, size_t>> pairs;
        pairs.reserve(n * (n - 1) / 2);
        for (size_t i = 0; i < n; ++i)
            for (size_t j = i + 1; j < n; ++j)
                pairs.emplace_back(i, j);

        pool_.parallel_for(pairs.begin(), pairs.end(), [&](auto& p) {
            matrix(p.first, p.second) = matcher_.run(
                templates[p.first], templates[p.second]);
        });
        return matrix;
    }
};

} // namespace iris
```

#### 3.3.7 Async Model Loading

```cpp
// biometric/iris/src/nodes/segmentation.cpp (initialization)
namespace iris {

OnnxSegmentation OnnxSegmentation::create(const SegmentationParams& params) {
    // Async: download model if not cached, load config in parallel
    auto model_future = AsyncIO::download_model(
        params.model_url, params.cache_dir);
    auto config_future = AsyncIO::load_yaml(params.config_path);

    // Both I/O operations proceed concurrently
    auto model_path = model_future.get();  // blocks until download/cache-hit
    auto config = config_future.get();

    // Load ONNX session (single load, no redundant onnx.load + check)
    Ort::SessionOptions options;
    options.SetIntraOpNumThreads(std::thread::hardware_concurrency());
    options.SetExecutionMode(ORT_PARALLEL);
    options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

    auto session = Ort::Session(ort_env(), model_path.c_str(), options);
    return OnnxSegmentation(std::move(session), config);
}

} // namespace iris
```

### 3.4 Core Data Types (C++23)

```cpp
// biometric/iris/include/iris/core/types.hpp

#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>
#include <array>
#include <optional>
#include <opencv2/core.hpp>

namespace iris {

enum class EyeSide : uint8_t { Left, Right };

struct IRImage {
    cv::Mat data;                   // CV_8UC1 grayscale
    std::string image_id;
    EyeSide eye_side;
};

struct SegmentationMap {
    cv::Mat predictions;            // CV_32FC4 (4-class probabilities)
    // Class indices: 0=eyeball, 1=iris, 2=pupil, 3=eyelashes
};

struct GeometryMask {
    cv::Mat pupil;                  // CV_8UC1 binary
    cv::Mat iris;
    cv::Mat eyeball;
};

struct NoiseMask {
    cv::Mat mask;                   // CV_8UC1 binary
};

struct Contour {
    std::vector<cv::Point2f> points;
};

struct GeometryPolygons {
    Contour pupil;
    Contour iris;
    Contour eyeball;
};

struct EyeCenters {
    cv::Point2d pupil_center;
    cv::Point2d iris_center;
};

struct EyeOrientation {
    double angle;                   // radians
};

struct NormalizedIris {
    cv::Mat normalized_image;       // CV_64FC1 (128 x res_phi)
    cv::Mat normalized_mask;        // CV_8UC1
};

struct IrisFilterResponse {
    struct Response {
        cv::Mat data;               // CV_64FC2 (real + imaginary)
        cv::Mat mask;               // CV_8UC1
    };
    std::vector<Response> responses;
};

struct IrisCode {
    cv::Mat code;                   // CV_8UC1 (16 x 256 x 2) -- one byte per bit (intermediate form)
    cv::Mat mask;                   // CV_8UC1 same shape
};

// See §16 for PackedIrisCode (bit-packed form used in IrisTemplate and matcher)

struct IrisTemplate {
    std::vector<PackedIrisCode> iris_codes;  // bit-packed for SIMD matching (§16)
    std::string version{"v2.0"};
};

// Extended template types for aggregation pipelines
struct IrisTemplateWithId : IrisTemplate {
    std::string image_id;               // source image identifier
};

struct WeightedIrisTemplate : IrisTemplate {
    std::vector<cv::Mat> weights;       // per-bit reliability weights (same shape as iris_codes)
};

struct AlignedTemplates {
    std::vector<IrisTemplateWithId> templates;
    DistanceMatrix distances;           // pairwise Hamming distances
    std::string reference_template_id;  // template used as alignment reference
};

struct Landmarks {
    std::vector<cv::Point2f> pupil_landmarks;
    std::vector<cv::Point2f> iris_landmarks;
    std::vector<cv::Point2f> eyeball_landmarks;
};

struct Offgaze { double score; };
struct Sharpness { double score; };
struct EyeOcclusion { double fraction; };

struct PupilToIrisProperty {
    double diameter_ratio;
    double center_distance_ratio;
};

struct BoundingBox {
    double x_min, y_min, x_max, y_max;
};

// Error handling
enum class ErrorCode : uint16_t {
    Ok = 0,
    SegmentationFailed,
    VectorizationFailed,
    GeometryEstimationFailed,
    NormalizationFailed,
    EncodingFailed,
    MatchingFailed,
    ValidationFailed,
    CryptoFailed,
    Cancelled,                          // cooperative cancellation (§13.2)
    ConfigInvalid,                      // YAML config parse/validation error (§15.5)
    ModelCorrupted,                     // checksum mismatch after download (§13.1.3)
    TemplateAggregationIncompatible,    // templates not compatible for aggregation
    IdentityValidationFailed,           // template consistency check failed
    KeyInvalid,                         // key validation failed (§6.6.4)
    KeyExpired,                         // key TTL exceeded (§6.6.5)
};

struct IrisError {
    ErrorCode code;
    std::string message;
    std::string detail;
};

template<typename T>
using Result = std::expected<T, IrisError>;

struct PipelineOutput {
    Result<IrisTemplate> iris_template;
    std::optional<IrisError> error;
    // Metadata map for debugging
};

} // namespace iris
```

### 3.5 Template Serialization Format

Python serializes `IrisTemplate` using `packbits` + base64 encoding. The C++ port must support both a native binary format and a Python-compatible exchange format for cross-language template exchange (§17.3.6).

```cpp
// biometric/iris/include/iris/utils/base64.hpp
namespace iris::utils {
    std::string base64_encode(std::span<const uint8_t> data);
    std::vector<uint8_t> base64_decode(std::string_view encoded);
}

// biometric/iris/include/iris/core/template_serializer.hpp
namespace iris {

class TemplateSerializer {
public:
    // Native binary format (compact, for C++ storage)
    static std::vector<uint8_t> to_binary(const IrisTemplate& tmpl);
    static Result<IrisTemplate> from_binary(std::span<const uint8_t> data);

    // Python-compatible format (base64 + packbits, for cross-language exchange)
    static std::string to_python_format(const IrisTemplate& tmpl);
    static Result<IrisTemplate> from_python_format(std::string_view data);

    // .npy format (for cross-language comparison tests)
    static void to_npy(const PackedIrisCode& code, const std::filesystem::path& path);
    static Result<PackedIrisCode> from_npy(const std::filesystem::path& path);
};

} // namespace iris
```

**Python format details**: The Python `IrisTemplate` supports two internal layouts -- a "new format" (list of `(H, W, 2)` arrays) and an "old format" (single `(H, W, N_filters, 2)` array). The C++ `from_python_format` must handle both for backwards compatibility. The `to_python_format` always emits the new format.

---

## 4. Technology Stack

### 4.1 Core Dependencies

| Library | Version | Purpose | License |
|---------|---------|---------|---------|
| **OpenCV** | 4.9+ | Image processing, contours, matrix ops | Apache 2.0 |
| **ONNX Runtime** | 1.17+ | Neural network inference for segmentation | MIT |
| **OpenFHE** | 1.4.2 | Fully homomorphic encryption | BSD 2-Clause |
| **yaml-cpp** | 0.8+ | YAML pipeline configuration parsing | MIT |
| **spdlog** | 1.14+ | Fast structured logging | MIT |
| **Google Test** | 1.15+ | Unit testing framework | BSD 3-Clause |
| **Google Benchmark** | 1.9+ | Performance benchmarking | Apache 2.0 |
| **nlohmann/json** | 3.11+ | JSON serialization | MIT |
| **Eigen** | 3.4+ | Linear algebra (ellipse fitting, SVD) | MPL2 |

### 4.2 Build Tools

| Tool | Purpose |
|------|---------|
| **CMake 3.28+** | Build system (with presets) |
| **Conan 2** | C++ package manager |
| **Ninja** | Fast build backend |
| **ccache** | Compilation cache |
| **clang-tidy** | Static analysis |
| **clang-format** | Code formatting |
| **cppcheck** | Additional static analysis |
| **sanitizers** | ASan, UBSan, TSan in debug builds |

### 4.3 Compiler Requirements

- **C++23 standard** (`-std=c++23`)
- **GCC 13+** or **Clang 17+** (for full C++23 support: `std::expected`, `std::format`, ranges, deducing this)
- OpenFHE requires C++17 minimum (compatible with C++23 builds)

---

## 5. Module Conversion Plan

### 5.1 Data Layer (`iris/io/` -> `biometric/iris/include/iris/core/`)

| Python | C++ | Notes |
|--------|-----|-------|
| `dataclasses.py` (Pydantic models) | `types.hpp` (structs with constructors) | Replace Pydantic validation with compile-time concepts and constructor checks |
| `errors.py` (Exception classes) | `error.hpp` (`std::expected` + `ErrorCode` enum) | No exceptions in hot paths |
| `class_configs.py` (Algorithm base) | `algorithm.hpp` (CRTP base) | Static dispatch via CRTP |
| `validators.py` (I/O validation) | Validation in constructors + concepts | Use C++23 concepts for type constraints |

### 5.2 Segmentation (`nodes/segmentation/`)

| Python | C++ | Notes |
|--------|-----|-------|
| `ONNXMultilabelSegmentation` | `OnnxSegmentation` class | ONNX Runtime C++ API directly |
| HuggingFace model download | `AsyncIO::download_model` with local cache | **Async**: non-blocking download with cache-first check |
| Model loaded twice (onnx.load + InferenceSession) | Single `Ort::Session` from memory buffer | **Fix**: eliminate redundant load |
| numpy tensor preprocessing | `cv::dnn::blobFromImage` or raw pointer | Zero-copy where possible |
| `MultilabelSegmentationBinarization` | `SegmentationBinarizer` | Threshold segmentation map per class |
| `SpecularReflectionDetection` | `SpecularReflectionDetector` | OpenCV threshold + morphology |

**Key algorithm**: Resize image to 640x480, normalize to [0,1], run UNet++ ONNX model, argmax over classes.

**Concurrency**: ONNX Runtime configured with `ORT_PARALLEL` execution mode and `SetIntraOpNumThreads(hardware_concurrency)`. Model download and config load happen concurrently via `AsyncIO`. The bilateral filter denoising uses OpenCV's internal threading.

### 5.3 Vectorization (`nodes/vectorization/`)

| Python | C++ | Notes |
|--------|-----|-------|
| `ContouringAlgorithm` | `ContourExtractor` | `cv::findContours` with area filter |

**Key algorithm**: `cv::findContours(mask, contours, RETR_TREE, CHAIN_APPROX_NONE)`, then select largest by area.

### 5.4 Geometry Refinement (`nodes/geometry_refinement/`)

| Python | C++ | Notes |
|--------|-----|-------|
| `ContourInterpolation` | `ContourInterpolator` | Linear interpolation between contour points |
| `ContourPointNoiseEyeballDistanceFilter` | `EyeballDistanceFilter` | Remove points too close/far from eyeball |
| `Smoothing` | `ContourSmoother` | Rolling median in polar coordinates |

**Key algorithms**:
- Interpolation: resample contour to N evenly-spaced points
- Distance filter: compute signed distance to eyeball contour, reject outliers
- Smoothing: convert to polar (r, theta), apply median filter on r, convert back

### 5.5 Geometry Estimation (`nodes/geometry_estimation/`)

| Python | C++ | Notes |
|--------|-----|-------|
| `LinearExtrapolation` | `LinearExtrapolator` | Polar-space extrapolation with dphi=0.703125 |
| `LSQEllipseFitWithRefinement` | `EllipseFitter` | Eigen-based least squares + refinement |
| `FusionExtrapolation` | `FusionExtrapolator` | Switch between methods based on std threshold |

**Key algorithms**:
- Linear: for each angle phi, extrapolate radius r from known contour points
- LSQ Ellipse: fit 5-parameter ellipse `(cx, cy, a, b, theta)` via SVD, then refine by removing outliers
- Fusion: compute std of residuals; if std < 0.014 use linear, else use ellipse

### 5.6 Eye Properties (`nodes/eye_properties_estimation/`)

| Python | C++ | Notes |
|--------|-----|-------|
| `BisectorsMethod` | `BisectorsEyeCenter` | 100 random bisector pairs, median intersection |
| `MomentOfArea` | `MomentOrientation` | `cv::moments` + atan2 |
| `EccentricityOffgazeEstimation` | `OffgazeEstimator` | Eccentricity from moments or ellipse |
| `OcclusionCalculator` | `OcclusionCalculator` | Angle-based iris visibility fraction |
| `PupilIrisPropertyCalculator` | `PupilIrisRatio` | Diameter + center distance ratios |
| `SharpnessEstimation` | `SharpnessEstimator` | `cv::Laplacian` variance |
| `IrisBBoxCalculator` | `BoundingBoxEstimator` | Min/max of iris contour |

### 5.7 Normalization (`nodes/normalization/`)

| Python | C++ | Notes |
|--------|-----|-------|
| `LinearNormalization` | `LinearNormalizer` | Daugman rubber-sheet with `cv::remap` |
| `NonlinearNormalization` | `NonlinearNormalizer` | Squared transformation; `NonlinearType` enum for Wyatt correction |
| `PerspectiveNormalization` | `PerspectiveNormalizer` | `cv::getPerspectiveTransform` + `cv::warpPerspective` |
| `normalization/utils.py:generate_iris_mask()` | `generate_iris_mask()` free function | Generates mask from extrapolated contours + noise mask via `cv::fillPoly` |
| `utils/common.py:contour_to_mask()` | `contour_to_mask()` free function | Converts polygon contour to binary mask; used by normalization |

**Key algorithm** (Linear/Daugman):
For each (r, theta) in output grid:
```
P(r, theta) = (1 - r) * pupil_boundary(theta) + r * iris_boundary(theta)
```
Map pixel from Cartesian image at point P.

**Concurrency**: Python uses a per-pixel list comprehension (~65K iterations) for Linear/Nonlinear and a triple-nested loop for Perspective. In C++:
- **Linear/Nonlinear**: Precompute the full remap table as `cv::Mat` of `(x,y)` coordinates, then call `cv::remap()` once. OpenCV parallelizes `remap` internally via TBB/OpenMP. Eliminates the Python loop entirely.
- **Perspective**: Use `cv::warpPerspective` per angle region (36 regions), with `parallel_for` over the independent regions. Each `warpPerspective` is internally SIMD-accelerated.

### 5.8 Iris Response / Feature Extraction (`nodes/iris_response/`)

| Python | C++ | Notes |
|--------|-----|-------|
| `ConvFilterBank` | `FilterBank` | Applies filters + probe schemas |
| `GaborFilter` | `GaborFilter` | Custom 2D Gabor kernel generation |
| `LogGaborFilter` | `LogGaborFilter` | FFT-based log-Gabor |
| `RegularProbeSchema` | `RegularProbeSchema` | 16x256 uniform grid |

**Key algorithm** (Gabor Filter):
```
kernel(x, y) = exp(-0.5 * (rotx²/σ_φ² + roty²/σ_ρ²)) * exp(j * 2π/λ_φ * rotx)
rotx = x*cos(θ) - y*sin(θ)
roty = x*sin(θ) + y*cos(θ)
```
With DC correction: subtract mean of envelope.
Output is complex-valued: `cv::filter2D` with real and imaginary kernels separately.

**C++ optimization opportunity**: Precompute kernels at compile time with `consteval` if parameters are known. Use OpenCV's `cv::filter2D` or manual SIMD convolution.

**Concurrency**: This is the **#1 compute bottleneck** in Python (nested 16x256 loop per filter = 8192 Python iterations for 2 filters). In C++:
- Replace the entire nested loop with `cv::filter2D` (single call, OpenCV internally parallelized + SIMD).
- The probe schema sampling becomes a simple `cv::remap` lookup on the convolution output.
- **Multiple filters run in parallel** via `parallel_for` over the filter list (default: 2 Gabor filters, fully independent).
- Log-Gabor uses `cv::dft` / `cv::idft` which are already parallelized internally.

### 5.9 Iris Response Refinement (`nodes/iris_response_refinement/`)

| Python | C++ | Notes |
|--------|-----|-------|
| `FragileBitRefinement` | `FragileBitRefiner` | Mark bits near decision threshold as fragile (masked) |

**Key algorithm**: For each filter response value, if `|value| < threshold`, mark corresponding mask bit as 0 (fragile/unreliable).

### 5.10 Encoder (`nodes/encoder/`)

| Python | C++ | Notes |
|--------|-----|-------|
| `IrisEncoder` | `IrisEncoder` | Sign-based binarization of complex response |

**Key algorithm**:
```
iris_code[i][0] = (real(response[i]) >= 0) ? 1 : 0
iris_code[i][1] = (imag(response[i]) >= 0) ? 1 : 0
```
Mask bits set to 1 only where `mask_value >= threshold` (default 0.9).

**C++ optimization**: Bit-pack the codes for cache efficiency. Use SIMD for sign extraction.

### 5.11 Matcher (`nodes/matcher/`)

| Python | C++ | Notes |
|--------|-----|-------|
| `HammingDistanceMatcher` | `HammingMatcher` | Full-featured rotation-compensating matcher |
| `SimpleHammingDistanceMatcher` | `SimpleHammingMatcher` | Basic HD |
| `BatchMatcher` | `BatchMatcher` | Gallery-scale matching |

**Key algorithm** (Hamming Distance):
```
For shift in [-rotation_shift, +rotation_shift]:
    xor_bits = iris_code_a XOR circular_shift(iris_code_b, shift)
    valid_mask = mask_a AND circular_shift(mask_b, shift)
    hd = popcount(xor_bits AND valid_mask) / popcount(valid_mask)
    best_hd = min(best_hd, hd)
```
With normalization: `norm_hd = 0.5 - (0.5 - hd) * sqrt(maskbitcount) * norm_gradient`

**C++ optimization**: `std::popcount` (C++20) with 64-bit processing. NEON `vcnt` on aarch64. This is the single hottest function for matching at scale.

**Concurrency**:
- **Single match**: The 31 rotation shifts are independent -- process all shifts in a SIMD-vectorized loop (pack 31 shifted codes, XOR all at once). For a single match this is fast enough without threading.
- **Batch 1-vs-N**: `parallel_for` over gallery entries on thread pool. Each thread gets its own SIMD popcount inner loop.
- **N-vs-N pairwise**: Flatten upper-triangle pairs into a work queue, `parallel_for` across pairs. For N=20, that's 190 independent pairs.

### 5.12 Template Operations (`nodes/templates_*`)

| Python | C++ | Notes |
|--------|-----|-------|
| `HammingDistanceBasedAlignment` | `TemplateAligner` | Find best rotation alignment |
| `MajorityVoteAggregation` | `MajorityVoteAggregator` | Bit-wise majority vote across captures |
| `TemplateIdentityFilter` | `TemplateIdentityFilter` | Reject inconsistent templates |

**Concurrency**: Alignment and identity filtering use O(N^2) pairwise Hamming distance. Python runs these as sequential nested loops. In C++:
- Flatten all (i, j) pairs from the upper triangle into a vector
- `parallel_for` over pairs on the thread pool
- Each pair's Hamming distance (with 31 rotation shifts) is independent
- For 10 templates: 45 pairs x 31 shifts = 1,395 operations -> completes in microseconds with parallelism

### 5.13 Validators (`nodes/validators/`)

All validators become simple predicate functions or validator classes returning `Result<void>`:

```cpp
namespace iris::validators {
    // Object validators
    Result<void> check_mask_size(const IrisTemplate&, size_t min_bits = 4096);
    Result<void> check_occlusion(const EyeOcclusion&, double min = 0.25);
    Result<void> check_offgaze(const Offgaze&, double max = 0.8);
    Result<void> check_sharpness(const Sharpness&, double min = 461.0);
    Result<void> check_pupil_iris_ratio(const PupilToIrisProperty&,
                                         double min_ratio = 0.1,
                                         double max_ratio = 0.7,
                                         double max_center = 0.9);
    Result<void> check_polygons_length(const GeometryPolygons&, size_t min_points);
    Result<void> check_templates_aggregation_compatible(
        std::span<const IrisTemplateWithId> templates);

    // Cross-object validators (require multiple inputs)
    Result<void> check_pupil_inside_iris(const GeometryPolygons&);
    Result<void> check_eye_centers_inside_image(const IRImage&, const EyeCenters&,
                                                 double min_distance_to_border = 0.0);
    Result<void> check_extrapolated_polygons_inside_image(
        const IRImage&, const GeometryPolygons&);
}
```

### 5.14 Orchestration (`orchestration/` -> `biometric/iris/src/core/`)

| Python | C++ | Notes |
|--------|-----|-------|
| YAML DAG definition | Same YAML format, parsed with yaml-cpp via `AsyncIO::load_yaml` | **Async** config load |
| `Environment` | `PipelineEnvironment` | Error strategy, QA gate, output builder |
| Sequential `for node in pipeline` loop | `PipelineExecutor` with DAG-level parallelism | **Parallel**: independent nodes run concurrently |
| Dynamic class resolution (`pydoc.locate`) | Compile-time node registry (factory pattern) | `std::unordered_map<string, factory_fn>` |
| Callback system | Observer pattern with `std::function` | Pre/post node execution hooks (thread-safe) |
| All intermediates held in memory | Move semantics + selective retention | Only keep data needed by downstream nodes |

**Concurrency**: This is where the biggest architectural improvement happens. Python runs all 24 nodes in a flat sequential loop. The C++ `PipelineExecutor`:
1. Performs topological sort on the DAG at init time
2. Groups nodes into execution levels (nodes in the same level have no dependencies on each other)
3. Executes each level's nodes concurrently via the thread pool
4. Uses `std::latch` as a barrier between levels
5. Identified parallel levels: L0 (segmentation + specular), L5 (orientation + centers), L9 (4 property estimators), L12 (sharpness + filter bank)

### 5.15 Pipeline (`pipelines/`)

| Python | C++ | Notes |
|--------|-----|-------|
| `IRISPipeline` | `IrisPipeline` | Main single-image pipeline with DAG-parallel execution |
| `MultiframeIrisPipeline` | `MultiframePipeline` | **Parallel**: all images processed concurrently, then aggregated |
| `TemplatesAggregationPipeline` | `AggregationPipeline` | Template fusion with parallel pairwise alignment |

---

## 6. OpenFHE Integration Plan

### 6.1 Encryption Architecture

> **Scope note**: Like open-iris (Python), this library is a **pure in-process library** with no database,
> no server, and no network layer. All template persistence is the caller's responsibility.
> The "key holder" vs "compute node" separation below describes two **usage patterns** of the
> same library API — not separate processes or services. A future client/server deployment
> would use these same primitives (see `FUTURE_TODO.md`).

```
                KEY HOLDER (has secret key)         COMPUTE NODE (public + eval keys only)
                ─────────────────────────────       ──────────────────────────────────────

 Enrollment:

 IR Image ──> Pipeline ──> IrisTemplate
                              |
                              v
                        [Pack into int vector]
                              |
                              v
                        [BFV Encrypt with
                         public key]
                              |
                              v
                        EncryptedTemplate ─────────> caller stores (file, DB, etc.)


 Matching:

 IR Image ──> Pipeline ──> IrisTemplate
                              |
                              v
                        [BFV Encrypt probe] ───────> Encrypted matching:
                                                       diff    = Sub(ct_probe, ct_gallery)
                                                       diff_sq = Mult(diff, diff)
                                                       mask_and = Mult(probe_mask, gallery_mask)
                                                       masked  = Mult(diff_sq, mask_and)
                                                       num     = EvalSum(masked)
                                                       denom   = EvalSum(mask_and)
                                                              |
                        EncryptedResult <──────────────────────┘
                              |
                              v
                        [Decrypt with
                         secret key]
                              |
                              v
                        HD = num / denom
                        (only scalar revealed)


 In-process (single library, both roles):

 IR Image ──> Pipeline ──> IrisTemplate
                              |
                         ┌────┴────┐
                         v         v
                  [Encrypt]   [Plaintext match]     ← caller chooses mode
                         |         |
                         v         v
                  EncryptedTemplate  HammingDistance
```

**Key separation rationale**: The key holder possesses the `SecretKey` and can encrypt/decrypt.
The compute node possesses only `PublicKey` + `EvalKey` and can perform homomorphic matching
but **cannot** decrypt templates or results. In a single-process deployment (the default for
this library), both roles are exercised by the same caller. The API is designed so that a
future client/server split requires no changes to the crypto primitives — only a transport
layer on top (see `FUTURE_TODO.md § API & Integration`).

### 6.2 Scheme Selection: BFV with SIMD Packing

**Why BFV** (not BinFHE/TFHE):
- Iris code = 2 filters x 16 rows x 256 cols x 2 bits = **16,384 bits**
- BFV with N=16384 gives **8192 SIMD slots** -- fits one full filter's code in a single ciphertext
- Total computation: **3 homomorphic operations** (subtract, multiply, sum)
- Estimated matching time: **< 200ms** (vs minutes for gate-by-gate BinFHE)

### 6.3 Implementation

```cpp
// biometric/iris/include/iris/crypto/fhe_context.hpp
namespace iris::crypto {

class FHEContext {
public:
    struct Params {
        uint32_t plaintext_modulus = 65537;    // prime for batching
        uint32_t multiplicative_depth = 2;      // diff^2 + mask multiply
        SecurityLevel security = HEStd_128_classic;
        uint32_t ring_dimension = 16384;        // 8192 slots
    };

    static FHEContext create(const Params& params = {});

    KeyPair generate_keys() const;
    void generate_rotation_keys(const SecretKey& sk);
    void generate_sum_keys(const SecretKey& sk);

    // Key serialization — see §6.6.2 KeyStore for full lifecycle
    std::vector<uint8_t> serialize_public_key(const PublicKey& pk) const;
    std::vector<uint8_t> serialize_eval_keys() const;
};

// biometric/iris/include/iris/crypto/encrypted_template.hpp
class EncryptedTemplate {
public:
    // One ciphertext per iris code in the template
    // Each ciphertext packs 16*256*2 = 8192 bits as integers in SIMD slots
    std::vector<Ciphertext> encrypted_codes;
    std::vector<Ciphertext> encrypted_masks;
    std::string version;

    // Serialization — caller handles persistence (file, network, etc.)
    std::vector<uint8_t> serialize() const;
    static EncryptedTemplate deserialize(std::span<const uint8_t> data);
};

// biometric/iris/include/iris/crypto/encrypted_matcher.hpp
class EncryptedMatcher {
public:
    struct Params {
        int32_t rotation_shift = 15;
        bool normalize = true;
        double norm_mean = 0.45;
        double norm_gradient = 5e-5;
    };

    // Returns encrypted Hamming distance (scalar in ciphertext).
    // Operates on public + eval keys only — no secret key needed.
    // Caller decrypts the result with secret key to obtain HD scalar.
    Ciphertext match_encrypted(
        const EncryptedTemplate& probe,
        const EncryptedTemplate& gallery
    ) const;

    // Batch matching: one probe vs N encrypted templates (caller-provided collection)
    std::vector<Ciphertext> batch_match(
        const EncryptedTemplate& probe,
        std::span<const EncryptedTemplate> gallery
    ) const;
};

} // namespace iris::crypto
```

### 6.4 Rotation Handling Under Encryption

The rotation shift (template alignment) is the main challenge under FHE. Two strategies:

**Strategy A - Pre-compute rotations (recommended for small shift range)**:
- For each shift in [-15, +15], rotate the gallery template (31 rotations)
- Pack all rotations as separate ciphertexts or within slots
- Compute all 31 Hamming distances, then take minimum
- Cost: 31 * 3 = 93 homomorphic operations, still < 1 second

**Strategy B - Probe-side rotation**:
- Encrypt probe at all 31 rotations before matching
- Match each rotated probe ciphertext against gallery
- Decrypt all 31 results, pick minimum
- Trade-off: more ciphertexts (31x), but simpler matching logic

### 6.5 Mask Handling Under Encryption

```cpp
// Encrypted matching (compute-node side — no secret key needed)
auto ct_diff = cc->EvalSub(ct_probe_code, ct_gallery_code);
auto ct_diff_sq = cc->EvalMult(ct_diff, ct_diff);           // XOR for binary
auto ct_mask_and = cc->EvalMult(ct_probe_mask, ct_gallery_mask); // AND masks
auto ct_masked = cc->EvalMult(ct_diff_sq, ct_mask_and);     // apply mask
auto ct_numerator = cc->EvalSum(ct_masked, num_bits);        // sum differences
auto ct_denominator = cc->EvalSum(ct_mask_and, num_bits);    // sum valid bits
// Both numerator and denominator returned encrypted
// Key holder decrypts and computes: HD = numerator / denominator
```

Multiplicative depth needed: 2 (one for diff^2, one for mask multiply).

### 6.6 Key Lifecycle & Secure Handling

FHE key management is critical for the security of the entire system. Unlike symmetric encryption where a single key is used, BFV requires **three distinct key types** with different sensitivity levels:

| Key Type | Sensitivity | Purpose | Storage Guidance |
|----------|-------------|---------|------------------|
| **Secret Key** (`SecretKey`) | **CRITICAL** — holder can decrypt all templates | Decryption of results; generation of evaluation keys | Never leaves key holder; zero memory after use; restrict file permissions (`0600`) |
| **Public Key** (`PublicKey`) | Low — public by design | Encryption of iris templates | Can be distributed freely; bundled with library if needed |
| **Evaluation Keys** (`EvalKey`) | Medium — enables computation but not decryption | Homomorphic operations (rotation, summation) | Share with compute node; large (~100MB); cache aggressively |

#### 6.6.1 Key Generation

```cpp
// biometric/iris/include/iris/crypto/key_manager.hpp
namespace iris::crypto {

struct KeyMetadata {
    std::string key_id;                       // UUID v4
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;  // optional
    SecurityLevel security_level;             // HEStd_128_classic, etc.
    uint32_t ring_dimension;
    uint32_t plaintext_modulus;
    std::string sha256_fingerprint;           // hash of serialized public key
};

class KeyManager {
public:
    struct KeyBundle {
        PublicKey public_key;
        SecretKey secret_key;
        EvalKey   rotation_keys;    // for EvalRotate (iris code shift)
        EvalKey   sum_keys;         // for EvalSum (Hamming distance accumulation)
        KeyMetadata metadata;
    };

    /// Generate a full key bundle for the given FHE context
    static KeyBundle generate(const FHEContext& ctx,
                              std::optional<std::chrono::hours> ttl = std::nullopt);

    /// Validate that a loaded key bundle is compatible with the given context
    static Result<void> validate(const KeyBundle& bundle, const FHEContext& ctx);

    /// Check if a key bundle has expired
    static bool is_expired(const KeyBundle& bundle);
};
```

#### 6.6.2 Key Serialization & Storage

All keys are serialized to files using OpenFHE's native serialization (binary or JSON). The library uses a **directory-based key store** layout:

```
keys/
├── key_meta.json          # KeyMetadata as JSON (not secret)
├── public_key.bin         # PublicKey (shareable)
├── secret_key.bin         # SecretKey (restrict permissions, CRITICAL)
├── rotation_keys.bin      # EvalKey for rotations (~50-100 MB)
└── sum_keys.bin           # EvalKey for summation
```

```cpp
class KeyStore {
public:
    /// Save a complete key bundle to directory.
    /// Sets file permissions: secret_key.bin -> 0600, others -> 0644.
    static Result<void> save(const KeyManager::KeyBundle& bundle,
                             const std::filesystem::path& dir);

    /// Load a complete key bundle from directory.
    /// Validates metadata and key-context compatibility.
    static Result<KeyManager::KeyBundle> load(
        const std::filesystem::path& dir,
        const FHEContext& ctx);

    /// Load only public + evaluation keys (compute-node scenario).
    /// Does NOT load secret key.
    struct ComputeKeys {
        PublicKey public_key;
        EvalKey rotation_keys;
        EvalKey sum_keys;
        KeyMetadata metadata;
    };
    static Result<ServerKeys> load_compute_keys(
        const std::filesystem::path& dir,
        const FHEContext& ctx);
};
```

#### 6.6.3 Secure Memory Handling

Secret key material requires special handling to prevent leakage:

- **Zero on destruction**: `SecretKey` wrapper overwrites memory with zeros in its destructor using `OPENSSL_cleanse`-style volatile writes (or `explicit_bzero` where available)
- **No swap**: Advise `mlock()` on secret key memory regions on Linux (best-effort; warn if `mlock` fails, do not abort)
- **No logging**: Secret key bytes must never appear in log output; `KeyMetadata` (which contains no secret material) is safe to log
- **No core dumps**: When loading secret keys, set `RLIMIT_CORE` to 0 for the process (best-effort)

```cpp
/// RAII wrapper that zeros memory on destruction
class SecureBuffer {
public:
    explicit SecureBuffer(size_t size);
    ~SecureBuffer();  // zeros memory via volatile write
    SecureBuffer(const SecureBuffer&) = delete;
    SecureBuffer& operator=(const SecureBuffer&) = delete;
    SecureBuffer(SecureBuffer&&) noexcept;
    SecureBuffer& operator=(SecureBuffer&&) noexcept;

    std::span<uint8_t> data() noexcept;
    std::span<const uint8_t> data() const noexcept;
};
```

#### 6.6.4 Key Validation & Compatibility

Before using loaded keys, validate:

1. **Metadata check**: `key_id` is non-empty, `created_at` is in the past, `expires_at` (if set) is in the future
2. **Context compatibility**: `ring_dimension` and `plaintext_modulus` in metadata match the `FHEContext` params
3. **Functional check**: encrypt a known test vector with the public key, decrypt with secret key, verify round-trip
4. **Fingerprint check**: SHA-256 of the serialized public key matches `sha256_fingerprint` in metadata

If any check fails, return `ErrorCode::KeyInvalid` with a descriptive message.

#### 6.6.5 Key Expiry

Keys include an optional TTL (time-to-live). When a key bundle expires:

- `KeyManager::is_expired()` returns `true`
- `KeyStore::load()` returns `ErrorCode::KeyExpired` (caller decides whether to proceed or re-generate)
- Templates encrypted with expired keys remain valid — expiry indicates the keys should be rotated, not that existing ciphertexts are invalid

The library does **not** automatically rotate keys or re-encrypt templates. Key rotation is a caller-driven operation:

```cpp
/// Re-encrypt a template with new keys (requires both old and new secret keys).
/// Decrypt with old_sk, re-encrypt with new_pk.
Result<EncryptedTemplate> re_encrypt(
    const EncryptedTemplate& old_template,
    const SecretKey& old_sk,
    const PublicKey& new_pk,
    const FHEContext& ctx);
```

HSM integration, key distribution protocols, and automated rotation policies are deferred to future work (see `FUTURE_TODO.md`).

---

## 7. Container Strategy

### 7.1 Multi-Stage Dockerfile

```
Stage 1: base-deps
  - Ubuntu 24.04
  - GCC 13+ / Clang 17+
  - CMake 3.28+, Ninja, ccache
  - OpenCV 4.9+ from source (with SIMD optimizations)
  - ONNX Runtime 1.17+ from prebuilt release
  - Eigen, yaml-cpp, spdlog, nlohmann-json, gtest, gbench

Stage 2: openfhe
  - Build OpenFHE v1.4.2 from source
  - Static libraries, release mode, native optimizations
  - Install to /usr/local

Stage 3: build
  - Copy source code
  - CMake configure + build
  - Run unit tests
  - Run integration tests

Stage 4: runtime (minimal)
  - Copy only compiled binary + shared libs + ONNX model
  - Non-root user
  - Hardened: no shell, no package manager
  - Health check endpoint
```

### 7.2 docker-compose.yml

```yaml
services:
  dev:
    build:
      context: ./biometric/iris
      target: build
    volumes:
      - ./biometric/iris/src:/app/src
      - ./biometric/iris/include:/app/include
      - ./biometric/iris/tests:/app/tests
      - ccache:/root/.ccache
    command: bash  # interactive development

  test:
    build:
      context: ./biometric/iris
      target: build
    command: ctest --output-on-failure

  bench:
    build:
      context: ./biometric/iris
      target: build
    command: ./build/tests/bench/bench_pipeline --benchmark_format=json

  prod:
    build:
      context: ./biometric/iris
      target: runtime
    ports:
      - "8080:8080"
    read_only: true
    security_opt:
      - no-new-privileges:true

volumes:
  ccache:
```

### 7.3 Development Workflow

```
Host machine:
  - Edit source in IDE (VSCode with remote container)
  - Git operations

Container (dev):
  - CMake configure/build (with ccache)
  - Run tests
  - Debug with GDB/LLDB
  - Profile with perf/valgrind

CI (GitHub Actions):
  - Build container from Dockerfile
  - Run full test suite
  - Run benchmarks
  - Publish container image
```

---

## 8. Build System

### 8.1 CMakeLists.txt (`biometric/iris/CMakeLists.txt`)

```cmake
cmake_minimum_required(VERSION 3.28)
project(iris_biometric VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(IRIS_BUILD_TESTS "Build test suite" ON)
option(IRIS_BUILD_BENCH "Build benchmarks" ON)
option(IRIS_ENABLE_FHE "Enable OpenFHE integration" ON)
option(IRIS_ENABLE_SANITIZERS "Enable ASan/UBSan" OFF)

# Dependencies
find_package(OpenCV 4.9 REQUIRED COMPONENTS core imgproc imgcodecs dnn)
find_package(Eigen3 3.4 REQUIRED)
find_package(yaml-cpp 0.8 REQUIRED)
find_package(spdlog 1.14 REQUIRED)
find_package(nlohmann_json 3.11 REQUIRED)

# ONNX Runtime (custom find module)
find_package(ONNXRuntime 1.17 REQUIRED)

# OpenFHE (optional but recommended)
if(IRIS_ENABLE_FHE)
    find_package(OpenFHE 1.4 REQUIRED)
endif()

# Library
add_library(iris
    src/core/types.cpp
    src/core/config.cpp
    src/core/pipeline.cpp
    src/nodes/segmentation.cpp
    src/nodes/binarization.cpp
    src/nodes/vectorization.cpp
    # ... all source files
)
target_include_directories(iris PUBLIC include)
target_link_libraries(iris PUBLIC
    opencv_core opencv_imgproc opencv_imgcodecs
    Eigen3::Eigen yaml-cpp spdlog::spdlog
    ${ONNXRUNTIME_LIBRARIES}
)
if(IRIS_ENABLE_FHE)
    target_link_libraries(iris PUBLIC ${OpenFHE_LIBRARIES})
    target_compile_definitions(iris PUBLIC IRIS_FHE_ENABLED)
endif()

# Tests
if(IRIS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
```

---

## 9. Implementation Phases

### Phase 1: Foundation (Weeks 1-2)

**Goal**: Build system, container, core types, and first algorithm node working end-to-end.

| Task | Details |
|------|---------|
| 1.1 Project scaffolding | CMakeLists.txt, CMakePresets.json (linux-release, linux-debug-sanitizers, linux-aarch64), directory structure, .clang-format, .clang-tidy |
| 1.2 Dockerfile | Multi-stage build with all dependencies |
| 1.3 Core types | All data structures in `types.hpp` including `PackedIrisCode` (§16) |
| 1.4 Error handling | `IrisError`, `Result<T>` types, `CancellationToken` (§13.2) |
| 1.5 Algorithm base | CRTP `Algorithm` base class |
| 1.6 Thread pool | `ThreadPool` with work-stealing, `pending_count()`, `active_count()`, sized to `hardware_concurrency()` |
| 1.7 Async I/O | `AsyncIO` with dedicated I/O thread pool, `DownloadPolicy` (retry + backoff), checksum verification (§13.1.3) |
| 1.8 Config parser + Node registry | YAML pipeline config loading with yaml-cpp; `NodeRegistry` with auto-registration macro (§15); validate all `class_name` entries at load time |
| 1.9 First node: Encoder | Simplest algorithm to verify the pattern works; register with `IRIS_REGISTER_NODE("iris.IrisEncoder", IrisEncoder)` |
| 1.10 SIMD popcount | Platform-dispatched `popcount_xor` with NEON primary target + scalar fallback (§14.2); benchmark |
| 1.11 NPY reader + comparison skeleton | `npy_reader.hpp` for reading Python .npy fixtures; comparison harness skeleton (§17.6) |
| 1.12 Unit tests | GTest for encoder, thread pool, async I/O, NPY reader, PackedIrisCode pack/unpack, SIMD popcount, cancellation token |

**Deliverable**: Container builds on Linux aarch64, thread pool and async I/O operational, encoder produces identical output to Python. NPY reader operational for cross-language fixtures. Node registry resolves class names from pipeline.yaml. NEON SIMD popcount benchmarked.

### Phase 2: Image Processing Nodes (Weeks 3-5)

**Goal**: All image processing and geometry nodes complete.

| Task | Details |
|------|---------|
| 2.1 Segmentation | ONNX Runtime inference, **async** model loading, `ORT_PARALLEL` execution mode |
| 2.2 Binarization | Segmentation map to binary masks |
| 2.3 Specular reflection | Threshold-based detection |
| 2.4 Vectorization | Contour extraction with OpenCV |
| 2.5 Contour interpolation | Uniform resampling |
| 2.6 Distance filter | Eyeball distance-based noise removal |
| 2.7 Smoothing | Polar-coordinate rolling median |
| 2.8 Linear extrapolation | Polar-space boundary extrapolation |
| 2.9 Ellipse fitting | Eigen-based LSQ fit + refinement |
| 2.10 Fusion extrapolation | Strategy selection based on std |
| 2.11 Generate Python reference data | Script to save intermediate outputs from Python pipeline |
| 2.12 Unit tests for all nodes | Compare against Python reference data |

**Deliverable**: All geometry processing nodes produce outputs matching Python within tolerance.

### Phase 3: Eye Properties & Normalization (Weeks 6-7)

**Goal**: Complete eye analysis and iris unwrapping.

| Task | Details |
|------|---------|
| 3.1 Bisectors method | Eye center estimation |
| 3.2 Moment of area | Orientation estimation |
| 3.3 Offgaze estimation | Eccentricity-based |
| 3.4 Occlusion calculator | Angle-based visibility |
| 3.5 Pupil/iris ratio | Diameter and center ratios |
| 3.6 Sharpness estimation | Laplacian variance |
| 3.7 Bounding box | Min/max contour bounds |
| 3.8 Linear normalization | Daugman rubber-sheet via `cv::remap` (replaces Python per-pixel loop) |
| 3.9 Nonlinear normalization | Squared transformation via `cv::remap` |
| 3.10 Perspective normalization | `cv::warpPerspective` per region + `parallel_for` over angle segments |
| 3.11 All validators | Threshold-based quality checks |
| 3.12 Unit tests + Python comparison | Numerical accuracy validation |

**Deliverable**: Full preprocessing chain from segmentation output to normalized iris image.

### Phase 4: Feature Extraction & Encoding (Weeks 8-9)

**Goal**: Gabor filters, iris encoding, and template generation.

| Task | Details |
|------|---------|
| 4.1 Gabor filter | 2D spatial Gabor kernel generation + convolution |
| 4.2 Log-Gabor filter | FFT-based frequency-domain filter |
| 4.3 Regular probe schema | 16x256 uniform sampling grid |
| 4.4 Filter bank | `cv::filter2D` + `cv::remap` probe, **parallel** over independent filters |
| 4.5 Fragile bit refinement | Near-threshold bit masking |
| 4.6 Iris encoder | Complex response -> 2-bit binarization |
| 4.7 Unit tests | Bit-exact comparison with Python for encoder output |
| 4.8 Iris code bit agreement test | Cross-language comparison: `popcount(cpp XOR python) == 0` for iris codes (§17.3.3) |

**Deliverable**: Given a normalized iris image, produce identical iris template to Python. Iris code bit agreement confirmed at 100%.

### Phase 5: Matching & Template Operations (Weeks 10-11)

**Goal**: Complete matching system with rotation compensation.

| Task | Details |
|------|---------|
| 5.1 Simple Hamming matcher | Basic HD with SIMD popcount (`std::popcount` / `__builtin_popcountll`) |
| 5.2 Full Hamming matcher | Rotation shift + normalization + half matching |
| 5.3 Batch matcher (1-vs-N) | `parallel_for` over gallery entries on thread pool |
| 5.4 Pairwise matcher (N-vs-N) | `parallel_for` over upper-triangle pairs |
| 5.5 Template alignment | Parallel pairwise HD for rotation optimization |
| 5.6 Majority vote aggregation | Bit-wise voting |
| 5.7 Template identity filter | Parallel consistency checking |
| 5.8 SIMD optimization | NEON `cnt` for inner loop; benchmark vs scalar |
| 5.9 Unit tests + benchmarks | Verify HD values match Python; benchmark 1-vs-1M throughput |
| 5.10 HD correlation + biometric accuracy | Cross-language Hamming distance correlation (Pearson r = 1.0); FMR/FNMR comparison vs Python (§17.3.1, §17.3.4) |

**Deliverable**: Matching produces identical Hamming distances to Python. Batch 1-vs-1M matching benchmarked at >1M comparisons/sec. HD correlation and biometric accuracy metrics verified against Python baselines.

### Phase 6: Pipeline Orchestration (Weeks 12-13)

**Goal**: Full DAG-based pipeline execution from YAML config.

| Task | Details |
|------|---------|
| 6.1 Node registry | Complete all `IRIS_REGISTER_NODE` entries from the mapping table (§15.3); verify all 30+ Python class names resolve |
| 6.2 Pipeline DAG builder | Parse YAML, resolve nested `class_name` params recursively (§15.4), wire inputs/outputs, compute dependency graph |
| 6.3 Config validation | Validate all class names, source_node references, no cycles, nested class resolution at load time (§15.5) |
| 6.4 Execution level scheduler | Topological sort -> group into parallel execution levels |
| 6.5 `PipelineExecutor` | Execute each level's nodes concurrently via thread pool, `std::latch` barriers between levels; accept `CancellationToken` (§13.2) |
| 6.6 Error management | Thread-safe `PipelineEnvironment` with raise vs store strategies; node failure propagation (§13.1.2) |
| 6.7 Output builders | Simple, debugging, serialized |
| 6.8 Callback system | Thread-safe pre/post node execution hooks |
| 6.9 IrisPipeline | Single-image pipeline with DAG-parallel execution; cancellation support |
| 6.10 MultiframePipeline | **Parallel** per-image processing with partial failure recovery (§13.1.1); `max_concurrent` and `min_successful` params |
| 6.11 AggregationPipeline | Template fusion with parallel pairwise alignment |
| 6.12 Template serialization | `AsyncIO::write_file` for non-blocking template persistence |
| 6.13 Integration tests | Full pipeline E2E with CASIA1 images from `data/`; verify parallel results match sequential; test cancellation mid-pipeline; test multiframe partial failure |
| 6.14 Full pipeline cross-language comparison | End-to-end comparison: all 24 nodes, iris codes, Hamming distances, match decisions (§17.3) |

**Deliverable**: `IrisPipeline` produces identical `IrisTemplate` to Python `IRISPipeline` for same input image. DAG-parallel execution verified. Multiframe processes N images concurrently with partial failure tolerance. Python `pipeline.yaml` loads unmodified. Full pipeline cross-language comparison passing on CASIA1 dataset.

### Phase 7: OpenFHE Integration (Weeks 14-16)

**Goal**: Encrypted template storage and matching.

| Task | Details |
|------|---------|
| 7.1 FHE context wrapper | BFV parameter setup, key generation |
| 7.2 Template encryption | Pack iris code bits into BFV ciphertext slots |
| 7.3 Template decryption | Unpack and verify round-trip correctness |
| 7.4 Encrypted matching | Sub/Mult/Sum for Hamming distance under encryption |
| 7.5 Rotation under encryption | Strategy A: pre-compute 31 rotated gallery ciphertexts |
| 7.6 Mask handling | Encrypted AND of masks + masked Hamming distance |
| 7.7 Key serialization | Public/private/evaluation key storage |
| 7.8 Encrypted template serialization | Ciphertext storage format |
| 7.9 Batch encrypted matching | One probe vs N encrypted gallery |
| 7.10 Key manager | `KeyManager::generate()`, `KeyManager::validate()`, `KeyManager::is_expired()`, configurable security level and TTL (§6.6.1) |
| 7.11 Key store | `KeyStore::save()`, `KeyStore::load()`, `KeyStore::load_compute_keys()`, directory-based layout, file permission enforcement (§6.6.2) |
| 7.12 Secure memory | `SecureBuffer` with volatile-zero destructor, `mlock` for secret key regions (§6.6.3) |
| 7.13 Key validation | Metadata check, context compatibility, functional round-trip check, fingerprint verification (§6.6.4) |
| 7.14 Key expiry & re-encryption | `is_expired()` check, `re_encrypt()` for template migration to new keys (§6.6.5) |
| 7.15 Unit tests | Verify encrypted HD matches plaintext HD; key lifecycle tests (generate, save, load, validate, expire); `SecureBuffer` zero-on-destroy test |
| 7.16 Performance benchmarks | Measure encryption, matching, decryption times; key generation time; key serialization/deserialization time |

**Deliverable**: Encrypted matching produces same decision (match/no-match) as plaintext matching. Sub-second single match. Complete key lifecycle (generate, store, load, validate, expire, re-encrypt) operational with secure memory handling.

### Phase 8: Hardening & Optimization (Weeks 17-18)

**Goal**: Production readiness.

| Task | Details |
|------|---------|
| 8.1 Memory optimization | Pool allocators, zero-copy where possible, move-only pipeline context |
| 8.2 SIMD audit | Ensure hot paths use NEON SIMD (Gabor convolution, Hamming distance, bit packing) on Linux aarch64 (§14.2) |
| 8.3 Thread pool tuning | Profile optimal thread count, test work-stealing fairness under load; verify saturation safeguards (§13.1.4) |
| 8.4 TSan pass | Full test suite under ThreadSanitizer -- verify all concurrent paths are race-free (including cancellation) |
| 8.5 ASan/UBSan pass | Full test suite under AddressSanitizer + UndefinedBehaviorSanitizer |
| 8.6 Async I/O stress test | Concurrent model downloads with retry/backoff (§13.1.3), parallel file writes, checksum verification, error recovery |
| 8.7 Pipeline concurrency test | Verify DAG-parallel results match sequential execution bit-for-bit; cancellation mid-pipeline; multiframe partial failure |
| 8.8 Fuzzing | Fuzz image input, YAML config (malformed class_names, circular deps), template deserialization |
| 8.9 Static analysis | clang-tidy, cppcheck, full warning sweep |
| 8.10 Security audit | Buffer overflows, integer overflow, timing attacks, lock contention |
| 8.11 API hardening | Input validation at all public boundaries |
| 8.12 Documentation | Doxygen for public API |
| 8.13 CI/CD pipeline | GitHub Actions: build (linux-release on aarch64), test (sequential + parallel modes), benchmark, publish container |
| 8.14 Platform verification | Verify builds and tests pass on Linux aarch64 (container); verify NEON SIMD dispatch correctness |
| 8.15 Complete comparison harness | `Dockerfile.comparison`, `cross_language_comparison.py`, CI integration, JSON report generation (§17.6); run on CASIA1 + CASIA-Iris-Thousand |
| 8.16 Comparison regression tracking | `comparison_history.jsonl` for tracking perf/accuracy regressions across PRs (§17.6) |

**Deliverable**: Production-hardened, fully tested (including concurrency verification, cancellation, partial failure recovery), container-published library for Linux aarch64. Complete cross-language comparison harness in CI with automated regression tracking on local CASIA datasets.

---

## 10. Test Plan

### 10.1 Test Strategy

```
               ┌─────────────────────────────┐
               │  Cross-Language Comparison   │  C++ vs Python: performance,
               │  Framework (§17)             │  accuracy, security, efficiency
               ├─────────────────────────────┤
               │    E2E Tests                │  Real IR images through
               │    (5 tests)                │  full pipeline
               ├─────────────────────────────┤
               │  Integration Tests          │  Multi-node sequences
               │  (15 tests)                 │  with real intermediate data
               ├─────────────────────────────┤
               │  Cross-Language             │  C++ output == Python output
               │  Validation (30+)           │  for same inputs
               ├─────────────────────────────┤
               │    Unit Tests               │  Each algorithm node
               │    (100+ tests)             │  isolated with mock data
               └─────────────────────────────┘
```

### 10.2 Unit Tests (Per Module)

| Module | Test Count | What is Tested |
|--------|------------|----------------|
| `core/types` | 12 | Construction, move semantics, serialization, `PackedIrisCode` pack/unpack round-trip, rotation correctness |
| `core/thread_pool` | 10 | Submit/wait, parallel_for, batch_submit, shutdown, exception propagation, `pending_count`/`active_count`, saturation behavior |
| `core/async_io` | 10 | Async file read/write, concurrent reads, download with cache, retry/backoff policy, checksum verification, error handling |
| `core/cancellation` | 4 | Token cancel/check, concurrent cancel+check, token with pipeline abort |
| `core/node_registry` | 6 | Register/lookup, unknown class_name error, nested class resolution, auto-registration macro, all pipeline.yaml names resolve |
| `core/simd_popcount` | 4 | Correctness vs scalar reference, known bit patterns, zero-word edge case, platform dispatch |
| `segmentation` | 5 | ONNX model loading (async), inference output shape/range |
| `binarization` | 5 | Threshold correctness, class separation |
| `vectorization` | 5 | Contour extraction, area filtering |
| `geometry_refinement` | 8 | Interpolation accuracy, smoothing, distance filter |
| `geometry_estimation` | 10 | Linear extrapolation, ellipse fit, fusion logic |
| `eye_properties` | 12 | Centers, orientation, offgaze, occlusion, sharpness |
| `normalization` | 8 | Rubber-sheet via cv::remap, perspective transform |
| `iris_response` | 10 | Gabor kernel shape, convolution via cv::filter2D, parallel filter execution |
| `encoder` | 5 | Binarization correctness, bit packing |
| `matcher` | 12 | HD computation, rotation shift, normalization, parallel batch, parallel pairwise |
| `template_ops` | 8 | Parallel alignment, aggregation, identity filter |
| `validators` | 10 | Each validator boundary condition |
| `pipeline` | 12 | DAG construction, execution level scheduling, parallel execution, sequential/parallel parity, cancellation mid-pipeline, multiframe partial failure, config validation (bad class_name, circular deps) |
| `crypto` | 8 | Encrypt/decrypt round-trip, key serialization |
| `encrypted_matcher` | 6 | Encrypted HD matches plaintext HD |
| **Total** | **~180** | |

### 10.3 Cross-Language Validation Tests

For each pipeline node, we produce **reference data** from the Python implementation:

```python
# tools/generate_test_fixtures.py
# Runs Python pipeline on test images and saves intermediate outputs

import iris
import numpy as np
import pickle

pipeline = iris.IRISPipeline(env=iris.IRISPipeline.DEBUGGING_ENVIRONMENT)
result = pipeline(ir_image)

# Save each intermediate result as binary numpy arrays
np.save("fixtures/segmentation_output.npy", result["segmentation"].predictions)
np.save("fixtures/geometry_polygons_iris.npy", result["vectorization"].iris_array)
np.save("fixtures/normalized_image.npy", result["normalization"].normalized_image)
np.save("fixtures/iris_code_0.npy", result["encoder"].iris_codes[0])
# ... etc for all intermediate outputs
```

C++ tests load these fixtures and compare:

```cpp
TEST(CrossLanguage, NormalizationMatchesPython) {
    auto python_output = load_npy("fixtures/normalized_image.npy");
    auto cpp_output = normalizer.run(image, noise_mask, contours, orientation);
    EXPECT_NEAR_MAT(cpp_output.normalized_image, python_output, 1e-6);
}
```

**Tolerance levels**:
- Integer/boolean outputs: exact match
- Floating point after convolution: relative error < 1e-5
- Geometry (contour points): Euclidean distance < 0.5 pixels
- Hamming distance: exact match (it's a ratio of integers)
- Iris code bits: exact match (deterministic encoding)

### 10.4 Integration Tests

| Test | Description |
|------|-------------|
| Segmentation -> Binarization -> Vectorization | Full front-end chain |
| Geometry chain | Vectorization -> Refinement -> Estimation |
| Normalization chain | Geometry + Image -> Normalized iris |
| Feature extraction chain | Normalized -> Filter -> Refine -> Encode |
| Full pipeline (sequential) | Image -> Template (compare with Python) |
| Full pipeline (DAG-parallel) | Same as above, verify output matches sequential mode bit-for-bit |
| Multi-frame pipeline (parallel) | N images processed concurrently -> Aggregated template matches sequential |
| Multi-frame partial failure | 1 corrupted image in batch of 10 -> 9 templates aggregated, failure logged |
| Pipeline cancellation | Cancel token fired mid-pipeline -> returns `ErrorCode::Cancelled`, no crash |
| Parallel batch matching | 1-vs-N and N-vs-N produce same distances as sequential |
| Async model load | Pipeline initializes correctly with async model download + config load |
| Async model load with retry | Simulate download failure -> retry succeeds on second attempt |
| Config validation | Invalid class_name in YAML -> clear error message at load time |
| Encrypted pipeline | Template -> Encrypt -> Match -> Decrypt |

### 10.5 End-to-End Tests

| Test | Description |
|------|-------------|
| Same-eye match (CASIA1) | Two images of same subject from `data/` -> HD < 0.35 |
| Different-eye reject (CASIA1) | Two different subjects from `data/` -> HD > 0.40 |
| Quality rejection | Low-quality image -> appropriate error |
| Full encrypted match | Same as above but with encrypted templates |
| Config compatibility | Python `pipeline.yaml` loads and runs without modification in C++ pipeline |
| Concurrent pipeline stress | 10 simultaneous pipeline invocations on different CASIA1 images produce correct results |
| Async I/O error recovery | Network failure during model download retries with backoff and falls back gracefully |
| Cancellation under load | Cancel 5 concurrent pipelines -> all return cleanly, no resource leaks |

### 10.6 Performance Benchmarks

| Benchmark | Target | Measurement |
|-----------|--------|-------------|
| **Single-image pipeline** | | |
| Segmentation inference | < 50ms | Per-image ONNX inference (ORT_PARALLEL) |
| Full pipeline (sequential) | < 200ms | Image to template, single-threaded baseline |
| Full pipeline (DAG-parallel) | < 120ms | Image to template, with parallel execution levels |
| **Multiframe pipeline** | | |
| Multiframe 10 images (sequential) | < 2000ms | Baseline: 10 x single pipeline |
| Multiframe 10 images (parallel) | < 400ms | All images processed concurrently + aggregation |
| **Matching** | | |
| Hamming distance (single pair) | < 1μs | Single template pair with rotation shift |
| Hamming distance (batch 1-vs-1M) | < 500ms | One probe vs 1M gallery, parallel on thread pool |
| Pairwise N-vs-N (N=1000) | < 5s | 499,500 pairs, parallelized |
| **Crypto** | | |
| Template encryption | < 50ms | Plaintext to ciphertext |
| Encrypted matching (single) | < 500ms | Single encrypted match |
| Batch encrypted matching | < 5s/1000 | One probe vs 1000 encrypted gallery |
| **I/O** | | |
| Async model load (cached) | < 20ms | ONNX model from disk (async) |
| Async config load | < 1ms | YAML config load (async) |
| Async template serialize | < 5ms | Template to disk (non-blocking) |
| **Memory** | | |
| Peak memory (pipeline) | < 200MB | Single pipeline instance |
| Peak memory (10 parallel) | < 500MB | 10 concurrent pipeline instances |
| Peak memory (encrypted match) | < 500MB | Including FHE keys |
| **Scaling** | | |
| Thread pool scaling (2-16 cores) | Near-linear | Batch matching throughput vs core count |
| Pipeline speedup (DAG-parallel) | > 1.3x | vs sequential, on 4+ core machine |
| Multiframe speedup (N images) | > 0.8*N x | vs sequential, on N+ core machine |

### 10.7 Security Tests

| Test | Description |
|------|-------------|
| ThreadSanitizer: full suite | Full test suite under TSan, zero data races |
| ThreadSanitizer: pipeline parallel | DAG-parallel pipeline under TSan |
| ThreadSanitizer: multiframe | N concurrent pipelines under TSan |
| ThreadSanitizer: batch matcher | Parallel batch matching under TSan |
| Fuzzing: image input | Random/malformed images don't crash |
| Fuzzing: YAML config | Malformed configs don't crash |
| Fuzzing: template deserialization | Corrupted templates handled gracefully |
| Timing attack resistance | Hamming distance is constant-time for matching |
| Memory safety | Full test suite under ASan + UBSan |
| Thread safety | Full test suite under TSan |
| FHE correctness | Encrypted results always match plaintext |
| Key isolation | Private keys never appear in public outputs |

---

## 11. Verification Plan

### 11.1 Correctness Verification Matrix

| Verification Method | Scope | Frequency |
|---------------------|-------|-----------|
| **Unit tests** | Every function/class | Every build (CI) |
| **Cross-language tests** | Every pipeline node | Every build (CI) |
| **Integration tests** | Multi-node chains | Every build (CI) |
| **E2E tests** | Full pipeline | Every PR |
| **Benchmark regression** | Performance | Weekly / every PR |
| **Sanitizer builds** | Memory/thread safety | Nightly |
| **Fuzz testing** | Input boundaries | Nightly (1hr sessions) |
| **Static analysis** | Code quality | Every PR |
| **Manual code review** | Security-critical paths | Every PR to main |

### 11.2 Acceptance Criteria Per Phase

#### Phase 1 (Foundation)
- [x] Container builds from scratch in < 30 minutes
- [x] `IrisEncoder` produces bit-exact output vs Python for 5 test cases
- [x] All core types compile and pass unit tests (including `PackedIrisCode` round-trip)
- [x] CMake builds with GCC 13 on Linux aarch64
- [ ] `NodeRegistry` resolves all class names from `pipeline.yaml` (resolves registered nodes; full pipeline.yaml coverage requires Phase 3-6 nodes)
- [x] SIMD `popcount_xor` produces identical results across all platform variants
- [x] `CancellationToken` works correctly under concurrent access
- [x] `AsyncIO::download_model` retries on failure with exponential backoff

#### Phase 2 (Image Processing)
- [ ] Segmentation ONNX inference produces same 4-class map (max element-wise diff < 1e-4) — *requires cross-language test with real model*
- [ ] Contour extraction matches Python contours (Hausdorff distance < 1 pixel) — *requires cross-language test*
- [ ] Ellipse fit parameters match Python (< 0.1% relative error) — *requires cross-language test*
- [ ] All geometry nodes pass cross-language tests — *requires Python reference fixtures*
- [x] All 13 algorithm nodes implemented, compiled, and unit-tested with synthetic data (106 tests)
- [x] ONNX segmentation node compiles conditionally (`IRIS_HAS_ONNXRUNTIME`) with full preprocessing pipeline
- [x] All nodes registered with correct Python class names via `IRIS_REGISTER_NODE`
- [x] `iris.hpp` facade header exposes all Phase 1+2 nodes
- [x] Docker build green with `-Werror -Wconversion -Wsign-conversion -Wdouble-promotion`

#### Phase 3 (Eye Properties & Normalization)
- [ ] Eye center estimation within 0.5 pixels of Python
- [ ] Normalization output matches Python (PSNR > 60dB)
- [ ] All validators produce same pass/fail as Python for test dataset
- [ ] Sharpness scores within 1% of Python

#### Phase 4 (Feature Extraction)
- [ ] Gabor filter kernels match Python (max diff < 1e-10)
- [ ] Filter response values match Python (relative error < 1e-5)
- [ ] Iris code bits are **identical** to Python for same normalized input
- [ ] Mask codes are **identical** to Python

#### Phase 5 (Matching)
- [ ] Hamming distance **identical** to Python for all test pairs
- [ ] Rotation shift finds same optimal shift as Python
- [ ] Batch matcher throughput > 1M comparisons/second
- [ ] Same match/no-match decisions as Python on test dataset

#### Phase 6 (Pipeline)
- [ ] Python `pipeline.yaml` loads without modification; all 30+ class names resolve via `NodeRegistry`
- [ ] Nested class_name params (FusionExtrapolation, ConvFilterBank) resolve correctly
- [ ] Full pipeline produces identical `IrisTemplate` for CASIA1 test images
- [ ] Multi-frame pipeline produces same aggregated template
- [ ] Multi-frame partial failure: 1 bad image out of 10 still produces valid aggregated template
- [ ] Cancellation mid-pipeline returns `ErrorCode::Cancelled` and cleans up
- [ ] Error handling matches Python behavior (same images fail/pass QA)
- [ ] Invalid YAML config (unknown class_name, circular deps) produces clear error at load time

#### Phase 7 (OpenFHE)
- [ ] Encrypt-decrypt round-trip preserves template exactly
- [ ] Encrypted HD matches plaintext HD for all test pairs
- [ ] Same match/no-match decisions under encryption
- [ ] Single encrypted match < 500ms
- [ ] Key serialization round-trip preserves functionality
- [ ] Key lifecycle: generate -> save -> load -> validate round-trip works
- [ ] `SecureBuffer` zeros memory on destruction (verified with ASan/valgrind)
- [ ] Expired keys rejected with `ErrorCode::KeyExpired`
- [ ] `re_encrypt()` produces template that matches with new keys

#### Phase 8 (Hardening)
- [ ] Zero ASan/UBSan/TSan findings
- [ ] Zero clang-tidy warnings at strict level
- [ ] 8-hour fuzz session with no crashes (including malformed YAML configs)
- [ ] Full pipeline < 200ms (excluding model loading)
- [ ] Peak memory < 200MB
- [ ] Builds and tests pass on Linux aarch64 (container)
- [ ] NEON SIMD popcount verified on Linux aarch64
- [ ] CASIA1 full dataset (757 images) cross-language comparison passing
- [ ] CASIA-Iris-Thousand (20K images) biometric accuracy baselines documented

### 11.3 Continuous Verification (CI Pipeline)

```yaml
# .github/workflows/ci.yml
on: [push, pull_request]

jobs:
  build-and-test:
    runs-on: ubuntu-latest
    container:
      image: ghcr.io/iriscpp/build:latest
    steps:
      - checkout
      - cmake configure (Release)
      - cmake build
      - ctest (unit + integration)
      - cross-language validation tests

  sanitizers:
    runs-on: ubuntu-latest
    container:
      image: ghcr.io/iriscpp/build:latest
    steps:
      - cmake configure (Debug, ASan+UBSan)
      - cmake build
      - ctest

  static-analysis:
    runs-on: ubuntu-latest
    steps:
      - clang-tidy
      - cppcheck

  benchmark:
    if: github.event_name == 'pull_request'
    runs-on: ubuntu-latest
    steps:
      - build release
      - run benchmarks
      - compare against baseline
      - post results as PR comment
```

### 11.4 Sign-off Checklist (Final Release)

- [ ] All 180+ unit tests passing
- [ ] All cross-language validation tests passing (on CASIA1 dataset)
- [ ] All integration tests passing (including cancellation, partial failure, config validation)
- [ ] All E2E tests passing (using local `data/` CASIA1 images)
- [ ] Benchmark targets met
- [ ] Zero sanitizer findings (both plaintext and encrypted modes)
- [ ] Zero static analysis warnings
- [ ] Builds and tests pass on Linux aarch64 (container)
- [ ] `NodeRegistry` resolves all class names from unmodified Python `pipeline.yaml`
- [ ] `PackedIrisCode` bit layout verified: pack → unpack round-trip is lossless
- [ ] Security review completed
- [ ] FHE correctness verified on 1000+ template pairs
- [ ] Container image published and tested
- [ ] API documentation generated
- **Cross-Language Comparison (§17)** — sign-off based on **C++ Plaintext vs Python**:
  - [ ] `signoff_pass == true` in comparison report (§17.6.3)
  - [ ] C++ Plaintext pipeline speedup >= 3x over Python (§17.2.1)
  - [ ] Biometric accuracy (FMR/FNMR) matches Python baseline — C++ Plaintext (§17.3.1)
  - [ ] Iris code bit-exact agreement: 100% (§17.3.3)
  - [ ] Hamming distance Pearson r == 1.0, max discrepancy == 0.0 (§17.3.4)
  - [ ] Template cross-compatibility: C++ ↔ Python HD matches (§17.3.6)
  - [ ] C++ Plaintext peak RSS < 200MB, < 0.5x Python (§17.2.1)
- **Encryption Correctness** — C++ Encrypted vs C++ Plaintext:
  - [ ] `encryption_correctness_pass == true` in comparison report (§17.6.3)
  - [ ] Encrypted matching decision agreement: 0 disagreements vs plaintext (§17.3.7)
  - [ ] Encrypted HD values within 1e-6 of plaintext (§17.3.7)
  - [ ] Encrypted template entropy > 7.99 bits/byte (§17.4)
  - [ ] No plaintext iris code leak in encrypted output (§17.4)
- **Encryption Overhead** (informational, documented but not gated):
  - [ ] Encryption overhead metrics documented in comparison report (§17.2.2)
  - [ ] Storage expansion ratio documented (§17.5.2)

---

## 12. Risk Assessment

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| ONNX model produces different results on CPU vs Python | High | Medium | Test with exact same model file; pin ONNX Runtime version; compare to Python output |
| Floating-point divergence between numpy and OpenCV/Eigen | Medium | High | Define acceptable tolerance per operation; use same precision (float64 for geometry) |
| OpenFHE compilation adds 15+ min to container build | Low | High | Multi-stage Docker with cached OpenFHE layer; pre-built base image |
| C++23 features not available in container compiler | High | Low | Pin GCC 13+ in Dockerfile; test with both GCC and Clang |
| Gabor filter numerical accuracy | High | Medium | Compare kernel values to Python at double precision; use same DC correction formula |
| OpenFHE BFV slot count insufficient for iris code | Medium | Low | Verify: 16384 ring dim gives 8192 slots; iris code has 8192 bits per filter. Exact fit. |
| Performance regression from FHE overhead | Medium | Medium | FHE is only for storage/matching, not pipeline; keep plaintext pipeline path |
| ONNX Runtime C++ API differences from Python API | Medium | Medium | Verify tensor layout (NCHW); test with same model inputs |
| Hamming distance popcount differs due to bit packing | High | Medium | Carefully verify bit layout matches Python; test with known bit patterns |

---

## 13. Error Recovery & Resilience

### 13.1 Pipeline Error Recovery

The pipeline must handle partial failures gracefully -- especially in multiframe and batch scenarios where one failure should not abort the entire operation.

#### 13.1.1 Multiframe Partial Failure

When processing N images in `MultiframePipeline`, individual image failures must not crash the batch:

```cpp
// biometric/iris/include/iris/pipeline/multiframe_pipeline.hpp
struct MultiframeResult {
    IrisTemplate aggregated_template;
    size_t images_processed;
    size_t images_failed;
    std::vector<std::pair<size_t, IrisError>> failures;  // (index, error)
};
```

**Policy**: Continue processing remaining images. Aggregate only successful templates. Fail the entire batch only if fewer than `min_successful` images produce valid templates (default: 1). Log all individual failures with image index.

#### 13.1.2 Pipeline Node Failure

When a pipeline node fails mid-execution:

| Failure Mode | Behavior | Recovery |
|-------------|----------|----------|
| Segmentation ONNX inference fails | Fatal -- no downstream processing possible | Return `IrisError::SegmentationFailed` immediately; skip remaining nodes |
| Validator callback rejects (e.g., occlusion too high) | QA gate -- configurable via `Environment` | `raise` strategy: propagate error. `store` strategy: record error, continue pipeline |
| Geometry estimation fails (degenerate contour) | Non-fatal if store strategy | Log warning, downstream nodes receive `std::nullopt` inputs and skip |
| Encoder produces too few mask bits | QA rejection | Return `IrisError::ValidationFailed` with detail |

The `PipelineEnvironment` controls behavior:

```cpp
struct PipelineEnvironment {
    enum class ErrorStrategy { Raise, Store };
    ErrorStrategy error_strategy = ErrorStrategy::Raise;
    bool qa_gating_enabled = true;
    // When store strategy: collect all errors, return them with partial output
    // When raise strategy: first error aborts pipeline
};
```

#### 13.1.3 Async I/O Error Recovery

| Operation | Failure | Retry Policy | Fallback |
|-----------|---------|-------------|----------|
| Model download (HTTP) | Network error / timeout | 3 retries with exponential backoff (1s, 4s, 16s) | Fail with `IrisError` if cache miss; succeed from cache if available |
| Model download (HTTP) | Corrupt file (checksum mismatch) | Re-download once, skip cache | Fail with `IrisError::ModelCorrupted` |
| File read (ONNX model) | File not found / permission | No retry (deterministic failure) | Return `IrisError` immediately |
| File write (template) | Disk full / permission | No retry | Return `IrisError`, caller decides |
| YAML config load | Parse error | No retry | Return `IrisError::ConfigInvalid` with line/column |

```cpp
// biometric/iris/include/iris/core/async_io.hpp (additions)
struct DownloadPolicy {
    uint32_t max_retries = 3;
    std::chrono::milliseconds initial_backoff{1000};
    double backoff_multiplier = 4.0;
    std::chrono::milliseconds max_backoff{60000};
    bool verify_checksum = true;
    std::string expected_sha256;  // empty = skip verification
};
```

#### 13.1.4 Thread Pool Saturation

When all worker threads are busy and new work is submitted:

**Policy**: `ThreadPool::submit` never blocks -- tasks queue in an unbounded `std::deque`. The pool drains the queue as workers become available.

**Safeguards**:
- `ThreadPool` exposes `pending_count()` for monitoring queue depth
- `MultiframePipeline` limits concurrent pipeline instances to `max_concurrent` (default: `thread_count()`) to prevent memory exhaustion from too many in-flight images
- `BatchMatcher` chunks work into batches sized to `thread_count() * 4` to maintain cache locality

```cpp
class ThreadPool {
public:
    // ... existing API ...
    size_t pending_count() const noexcept;   // tasks queued but not started
    size_t active_count() const noexcept;    // tasks currently running
};

class MultiframePipeline {
public:
    struct Params {
        size_t max_concurrent = 0;  // 0 = thread_count()
        size_t min_successful = 1;  // minimum templates for aggregation
    };
};
```

### 13.2 Cancellation

`std::future` has no native cancellation. The pipeline uses a cooperative cancellation token:

```cpp
// biometric/iris/include/iris/core/cancellation.hpp
namespace iris {

class CancellationToken {
public:
    void cancel() noexcept;
    [[nodiscard]] bool is_cancelled() const noexcept;
};

// Pipeline nodes check the token between major operations:
// if (token.is_cancelled()) return IrisError{ErrorCode::Cancelled, "Pipeline cancelled"};

} // namespace iris
```

**Usage**: `IrisPipeline::run(image, token)` and `MultiframePipeline::run(images, token)` accept an optional token. The `PipelineExecutor` checks the token before launching each execution level. Individual nodes check at natural boundaries (e.g., between ONNX inference and postprocessing).

---

## 14. Platform

### 14.1 Target Platform

| Platform | Architecture | SIMD | Status |
|----------|-------------|------|--------|
| Linux (Ubuntu 24.04) | aarch64 (ARM64) | NEON | **Primary and only target** — container builds, CI, production |

**Development environment**: macOS Apple Silicon (M4) — used for editing and running Docker builds targeting Linux aarch64. No native macOS builds are produced or supported.

**Not targeted**: x86_64, macOS native, Windows. The AVX2/AVX-512 SIMD code in `matcher_simd.hpp` is retained for compile-time compatibility but is not the target instruction set.

### 14.2 SIMD Strategy (NEON Primary)

Three layers of SIMD optimization:

1. **OpenCV** handles SIMD internally for `filter2D`, `remap`, `warpPerspective`, `findContours`, etc. -- this covers ~80% of compute-heavy code. OpenCV's NEON support is mature on aarch64.

2. **Compiler auto-vectorization** with `-march=native` (container builds on ARM). Write loops in a vectorization-friendly style (contiguous memory, no data-dependent branches).

3. **Explicit NEON for Hamming distance popcount** -- the single hottest loop:

```cpp
// biometric/iris/include/iris/nodes/matcher_simd.hpp
namespace iris::simd {

// Primary target: ARM NEON with cnt instruction (128 bits per iteration)
#if defined(__ARM_NEON)
    size_t popcount_xor(const uint64_t* a, const uint64_t* b, size_t n_words);
#else
    // Scalar fallback using std::popcount (C++20)
    size_t popcount_xor(const uint64_t* a, const uint64_t* b, size_t n_words);
#endif

// Note: AVX2/AVX-512 variants are retained in the source for compile-time
// compatibility but are not the target instruction set.

} // namespace iris::simd
```

### 14.3 Build Presets

```json
// biometric/iris/CMakePresets.json
{
  "configurePresets": [
    {
      "name": "linux-release",
      "generator": "Ninja",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "CMAKE_CXX_FLAGS": "-march=native"
      }
    },
    {
      "name": "linux-debug-sanitizers",
      "generator": "Ninja",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "IRIS_ENABLE_SANITIZERS": "ON"
      }
    },
    {
      "name": "linux-debug-tsan",
      "generator": "Ninja",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "IRIS_ENABLE_TSAN": "ON"
      }
    },
    {
      "name": "linux-aarch64",
      "generator": "Ninja",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "CMAKE_CXX_FLAGS": "-march=armv8.2-a+crypto"
      }
    }
  ]
}
```

### 14.4 Development Environment Notes

- **Dev machine**: macOS Apple Silicon (M4) — editing and Docker builds only. No native macOS builds.
- **Docker**: All builds run in Docker targeting `linux/arm64`. On macOS Apple Silicon, Docker runs ARM containers natively (no emulation).
- **OpenFHE**: Built from source in Docker (stage 2 of Dockerfile). v1.4.2, static, release.
- **ONNX Runtime**: Prebuilt `aarch64` binaries downloaded in Dockerfile.

---

## 15. YAML Config Compatibility & Node Registry

### 15.1 The Compatibility Problem

Python resolves pipeline nodes at runtime via `pydoc.locate(class_name)` where `class_name` is a fully-qualified Python path like `iris.LinearNormalization` or `iris.nodes.validators.cross_object_validators.EyeCentersInsideImageValidator`. C++ has no runtime class lookup, so we need a compile-time registry that maps these exact Python class names to C++ factory functions.

### 15.2 Node Registry Design

```cpp
// biometric/iris/include/iris/core/node_registry.hpp
namespace iris {

// Factory function type: takes YAML params node, returns type-erased algorithm
using NodeFactory = std::function<std::unique_ptr<PipelineNodeBase>(const YAML::Node& params)>;

class NodeRegistry {
public:
    // Register a factory under one or more names
    void register_node(std::string_view class_name, NodeFactory factory);

    // Lookup by Python class_name from YAML
    Result<NodeFactory> lookup(std::string_view class_name) const;

    // Get the singleton global registry
    static NodeRegistry& instance();

private:
    std::unordered_map<std::string, NodeFactory> factories_;
};

// Auto-registration macro
#define IRIS_REGISTER_NODE(python_class_name, CppClass) \
    static const bool _reg_##CppClass = [] { \
        NodeRegistry::instance().register_node( \
            python_class_name, \
            [](const YAML::Node& params) -> std::unique_ptr<PipelineNodeBase> { \
                return std::make_unique<CppClass>(CppClass::from_yaml(params)); \
            }); \
        return true; \
    }()

} // namespace iris
```

### 15.3 Complete Python-to-C++ Class Name Mapping

Every `class_name` that appears in `pipeline.yaml` must have a registry entry. This is the exhaustive mapping:

| YAML `class_name` (Python) | C++ Class | Notes |
|----------------------------|-----------|-------|
| **Algorithm Nodes** | | |
| `iris.MultilabelSegmentation.create_from_hugging_face` | `OnnxSegmentation` | Factory method maps to constructor |
| `iris.MultilabelSegmentationBinarization` | `SegmentationBinarizer` | |
| `iris.ContouringAlgorithm` | `ContourExtractor` | |
| `iris.SpecularReflectionDetection` | `SpecularReflectionDetector` | |
| `iris.ContourInterpolation` | `ContourInterpolator` | |
| `iris.ContourPointNoiseEyeballDistanceFilter` | `EyeballDistanceFilter` | |
| `iris.MomentOfArea` | `MomentOrientation` | |
| `iris.BisectorsMethod` | `BisectorsEyeCenter` | |
| `iris.Smoothing` | `ContourSmoother` | |
| `iris.FusionExtrapolation` | `FusionExtrapolator` | Has nested `class_name` params |
| `iris.LinearExtrapolation` | `LinearExtrapolator` | Nested inside FusionExtrapolation |
| `iris.LSQEllipseFitWithRefinement` | `EllipseFitter` | Nested inside FusionExtrapolation |
| `iris.PupilIrisPropertyCalculator` | `PupilIrisRatio` | |
| `iris.EccentricityOffgazeEstimation` | `OffgazeEstimator` | |
| `iris.OcclusionCalculator` | `OcclusionCalculator` | Instantiated twice (90deg, 30deg) |
| `iris.NoiseMaskUnion` | `NoiseMaskAggregator` | |
| `iris.LinearNormalization` | `LinearNormalizer` | |
| `iris.NonlinearNormalization` | `NonlinearNormalizer` | Not in default config, but must be registered |
| `iris.PerspectiveNormalization` | `PerspectiveNormalizer` | Not in default config, but must be registered |
| `iris.SharpnessEstimation` | `SharpnessEstimator` | |
| `iris.ConvFilterBank` | `FilterBank` | Has nested filter + probe_schema lists |
| `iris.GaborFilter` | `GaborFilter` | Nested inside ConvFilterBank |
| `iris.LogGaborFilter` | `LogGaborFilter` | Not in default config |
| `iris.RegularProbeSchema` | `RegularProbeSchema` | Nested inside ConvFilterBank |
| `iris.nodes.iris_response_refinement.fragile_bits_refinement.FragileBitRefinement` | `FragileBitRefiner` | Long-form class name |
| `iris.IrisEncoder` | `IrisEncoder` | |
| `iris.IrisBBoxCalculator` | `BoundingBoxEstimator` | |
| **Validator Callbacks** | | |
| `iris.nodes.validators.cross_object_validators.EyeCentersInsideImageValidator` | `EyeCentersInsideImageValidator` | |
| `iris.nodes.validators.object_validators.Pupil2IrisPropertyValidator` | `Pupil2IrisPropertyValidator` | |
| `iris.nodes.validators.object_validators.OffgazeValidator` | `OffgazeValidator` | |
| `iris.nodes.validators.object_validators.OcclusionValidator` | `OcclusionValidator` | |
| `iris.nodes.validators.object_validators.SharpnessValidator` | `SharpnessValidator` | |
| `iris.nodes.validators.object_validators.IsMaskTooSmallValidator` | `IsMaskTooSmallValidator` | |
| `iris.nodes.validators.cross_object_validators.IsPupilInsideIrisValidator` | `IsPupilInsideIrisValidator` | |
| `iris.nodes.validators.cross_object_validators.ExtrapolatedPolygonsInsideImageValidator` | `ExtrapolatedPolygonsInsideImageValidator` | |
| `iris.nodes.validators.object_validators.PolygonsLengthValidator` | `PolygonsLengthValidator` | Minimum contour completeness |
| `iris.nodes.validators.object_validators.AreTemplatesAggregationCompatible` | `TemplateAggregationCompatibilityValidator` | Used by aggregation pipeline |
| **Matcher Nodes** (not in pipeline.yaml but used separately) | | |
| `iris.HammingDistanceMatcher` | `HammingMatcher` | |
| `iris.SimpleHammingDistanceMatcher` | `SimpleHammingMatcher` | |
| `iris.BatchMatcher` | `BatchMatcher` | |
| **Template Ops** | | |
| `iris.HammingDistanceBasedAlignment` | `TemplateAligner` | |
| `iris.MajorityVoteAggregation` | `MajorityVoteAggregator` | |
| `iris.TemplateIdentityFilter` | `TemplateIdentityFilter` | |

### 15.4 Nested Class Resolution

The YAML config uses nested `class_name`/`params` structures for algorithm parameters (e.g., `FusionExtrapolation` contains two nested algorithm specs, `ConvFilterBank` contains lists of filters and probe schemas). The `NodeRegistry` must handle recursive resolution:

```cpp
// When parsing a YAML params node, detect nested class references:
YAML::Node resolve_nested_params(const YAML::Node& params, const NodeRegistry& registry) {
    YAML::Node resolved = params;
    for (auto it = resolved.begin(); it != resolved.end(); ++it) {
        auto value = it->second;
        if (value.IsMap() && value["class_name"]) {
            // This parameter is a nested class -- resolve it
            auto class_name = value["class_name"].as<std::string>();
            auto nested_params = value["params"] ? value["params"] : YAML::Node{};
            // Store both the factory and resolved params for the parent to use
        }
        if (value.IsSequence()) {
            // Check each element for class references (e.g., filters list)
            for (size_t i = 0; i < value.size(); ++i) {
                if (value[i].IsMap() && value[i]["class_name"]) {
                    // Resolve nested class in list
                }
            }
        }
    }
    return resolved;
}
```

### 15.5 Config Validation at Load Time

Since Python uses Pydantic for runtime validation and C++ resolves at startup, all config errors must be caught early:

```cpp
Result<PipelineDAG> PipelineDAG::from_yaml(const std::filesystem::path& config_path) {
    auto yaml = YAML::LoadFile(config_path.string());

    // 1. Validate metadata
    if (!yaml["metadata"] || !yaml["pipeline"])
        return IrisError{ErrorCode::ConfigInvalid, "Missing metadata or pipeline key"};

    // 2. For each node: verify class_name exists in registry
    for (const auto& node_yaml : yaml["pipeline"]) {
        auto class_name = node_yaml["algorithm"]["class_name"].as<std::string>();
        if (!NodeRegistry::instance().lookup(class_name))
            return IrisError{ErrorCode::ConfigInvalid,
                std::format("Unknown class_name '{}' -- not in node registry", class_name)};

        // 3. Verify all source_node references point to existing nodes
        // 4. Verify no circular dependencies
        // 5. Resolve nested class_name params recursively
    }

    // 6. Build DAG, compute topological sort
    return dag;
}
```

---

## 16. Iris Code Bit-Packing Layout

### 16.1 The Problem

The Python implementation stores iris codes as numpy boolean arrays of shape `(16, 256, 2)` -- one byte per bit. The current `types.hpp` defines `IrisCode.code` as `cv::Mat CV_8UC1` with the same layout. This wastes 8x memory and prevents efficient SIMD popcount for Hamming distance.

### 16.2 Bit-Packed Layout

Each iris code has `16 * 256 * 2 = 8,192 bits` = **1,024 bytes** = **128 uint64_t words**.

```cpp
// biometric/iris/include/iris/core/iris_code_packed.hpp
namespace iris {

struct PackedIrisCode {
    // Contiguous bit-packed storage: 8192 bits = 128 x uint64_t
    alignas(64) std::array<uint64_t, 128> code_bits;
    alignas(64) std::array<uint64_t, 128> mask_bits;

    // Bit indexing: bit at (row, col, channel) maps to:
    //   linear_index = row * 256 * 2 + col * 2 + channel
    //   word = linear_index / 64
    //   bit  = linear_index % 64

    static constexpr size_t total_bits = 8192;
    static constexpr size_t n_words = 128;

    // Pack from cv::Mat (CV_8UC1, shape 16x256x2 with one byte per bit)
    static PackedIrisCode from_mat(const cv::Mat& code, const cv::Mat& mask);

    // Unpack to cv::Mat (for cross-language comparison / serialization)
    std::pair<cv::Mat, cv::Mat> to_mat() const;

    // Circular shift by `shift` columns (each column = 2 bits)
    PackedIrisCode rotate(int shift) const;
};

} // namespace iris
```

### 16.3 Hamming Distance on Packed Codes

```cpp
namespace iris {

// Core Hamming distance on packed codes -- uses SIMD popcount from §14.2
inline double hamming_distance(const PackedIrisCode& a, const PackedIrisCode& b) {
    size_t xor_popcount = 0;
    size_t mask_popcount = 0;

    for (size_t i = 0; i < PackedIrisCode::n_words; ++i) {
        uint64_t valid = a.mask_bits[i] & b.mask_bits[i];
        uint64_t diff  = a.code_bits[i] ^ b.code_bits[i];
        xor_popcount  += std::popcount(diff & valid);
        mask_popcount += std::popcount(valid);
    }

    if (mask_popcount == 0) return 1.0;  // no valid bits
    return static_cast<double>(xor_popcount) / static_cast<double>(mask_popcount);
}

// With SIMD (calls simd::popcount_xor from §14.2)
double hamming_distance_simd(const PackedIrisCode& a, const PackedIrisCode& b);

} // namespace iris
```

### 16.4 Rotation (Circular Column Shift)

Template matching requires rotating one code by up to +/-15 column positions. Each column is 2 bits wide, so a shift of `s` columns = `2s` bit positions.

```cpp
PackedIrisCode PackedIrisCode::rotate(int shift) const {
    if (shift == 0) return *this;

    PackedIrisCode result;
    const int bit_shift = shift * 2;  // 2 bits per column

    // Shift is applied per-row (each row = 256 cols * 2 ch = 512 bits = 8 words)
    constexpr size_t words_per_row = 8;  // 512 bits / 64

    for (size_t row = 0; row < 16; ++row) {
        const size_t base = row * words_per_row;
        // Circular bit-shift within the 512-bit row
        circular_shift_512(&code_bits[base], &result.code_bits[base], bit_shift);
        circular_shift_512(&mask_bits[base], &result.mask_bits[base], bit_shift);
    }
    return result;
}
```

### 16.5 Dual Representation Strategy

The pipeline internally uses `cv::Mat` for intermediate processing (encoder output, cross-language comparison). At the **template boundary** (output of `IrisEncoder`, input to `HammingMatcher`), codes are packed:

```
Encoder → cv::Mat (16,256,2) → PackedIrisCode::from_mat() → PackedIrisCode
                                                                    ↓
                                                            stored in IrisTemplate
                                                                    ↓
                                                            HammingMatcher uses packed form
```

For cross-language tests, `PackedIrisCode::to_mat()` unpacks back to `cv::Mat` for `.npy` comparison.

```cpp
struct IrisTemplate {
    std::vector<PackedIrisCode> iris_codes;  // packed for matching
    std::string version{"v2.0"};
};
```

---

## 17. Cross-Language Comparison Framework (C++ vs Python)

### 17.1 Overview & Architecture

The Python open-iris serves as the ground-truth reference. The C++ port must prove it is **equal in accuracy, superior in performance, stronger in security, and leaner in efficiency**. This framework automates that proof across 5 dimensions.

#### 17.1.1 Encryption-Aware Dual-Mode Testing

The C++ port has **FHE encryption enabled by default** — this is a deliberate design choice for production security. Python has no encryption capability. This creates an asymmetry that the comparison framework must handle explicitly:

**Three execution modes compared**:

| Mode | Pipeline | Matching | Template Storage | Purpose |
|------|----------|----------|-----------------|---------|
| **Python** | Plaintext | Plaintext HD | Plaintext numpy | Ground-truth reference |
| **C++ Plaintext** (`--no-encrypt`) | Plaintext | Plaintext HD (SIMD popcount) | Plaintext bit-packed | Apples-to-apples comparison vs Python |
| **C++ Encrypted** (default) | Plaintext | Encrypted HD (BFV homomorphic) | Encrypted ciphertext | Production mode, encryption overhead measured |

**Key principle**: The iris feature extraction pipeline (segmentation → normalization → encoding) is **always plaintext** in both C++ modes — FHE applies only to template storage and matching. Therefore:
- **Accuracy sign-off**: Based on **C++ Plaintext vs Python** (identical operations, must produce identical results)
- **Performance sign-off**: Based on **C++ Plaintext vs Python** (fair comparison without encryption overhead)
- **Encrypted mode**: Tracked separately as **informational metrics** + correctness verification (encrypted matching must produce the same match/no-match decisions as plaintext matching)

```
┌──────────────────────────────────────────────────────────────────────┐
│               cross_language_comparison.py (orchestrator)             │
│                                                                      │
│  ┌─────────────────┐  ┌─────────────────────┐  ┌─────────────────┐ │
│  │  Python Runner   │  │  C++ Plaintext      │  │  C++ Encrypted  │ │
│  │  (subprocess)    │  │  (--no-encrypt)     │  │  (default mode) │ │
│  │                  │  │                     │  │                 │ │
│  │  bench_python_*  │  │  iris_comparison    │  │  iris_comparison│ │
│  │  → .npy outputs  │  │  → .npy outputs    │  │  → .npy outputs │ │
│  │  → python.json   │  │  → cpp_plain.json  │  │  → cpp_enc.json │ │
│  └───────┬──────────┘  └────────┬────────────┘  └───────┬─────────┘ │
│          └──────────────┬───────┴─────────────────┬─────┘           │
│                         ▼                         ▼                  │
│                 Comparison Engine                                     │
│  ┌──────────────────────────────┐  ┌────────────────────────────┐   │
│  │  SIGN-OFF COMPARISON         │  │  INFORMATIONAL COMPARISON  │   │
│  │  Python vs C++ Plaintext     │  │  C++ Plaintext vs Encrypted│   │
│  │                              │  │                            │   │
│  │  • Performance (pass/fail)   │  │  • Encryption overhead     │   │
│  │  • Accuracy (pass/fail)      │  │  • Decision agreement      │   │
│  │  • Efficiency (pass/fail)    │  │  • Template size impact    │   │
│  └──────────────┬───────────────┘  └─────────────┬──────────────┘   │
│                 └───────────┬────────────────────┘                   │
│                             ▼                                        │
│                   comparison_report.json                              │
│                   (sign-off: plaintext pass/fail)                     │
│                   (encryption: informational + correctness)           │
└──────────────────────────────────────────────────────────────────────┘
```

**Execution environment**: A dedicated `Dockerfile.comparison` container that has both Python open-iris (pip-installed) and the C++ `iris_comparison` binary. This ensures identical ONNX Runtime version, same model file, same CPU — eliminating platform variance.

**CI integration**: Runs on every PR via `.github/workflows/comparison.yml`. Posts a summary table as a PR comment. Gates merge on `overall_pass == true` (based on plaintext comparison only). Encrypted mode results are reported as an informational supplement.

### 17.2 Head-to-Head Performance Comparison

Python baselines (from SEMSEG_MODEL_CARD.md and empirical measurement):
- ONNX CPU segmentation inference: **193ms**
- Full pipeline (estimated): **~850ms** (segmentation + all 23 other nodes)

All C++ performance metrics are measured in **two modes**: plaintext (`--no-encrypt`) and encrypted (default). **Pass/fail thresholds apply only to C++ Plaintext vs Python.** Encrypted mode metrics are informational — they quantify the FHE overhead.

#### 17.2.1 Metrics & Thresholds (Sign-off: C++ Plaintext vs Python)

| Metric | Measurement Method | Pass Threshold (Plaintext) |
|--------|-------------------|----------------|
| **Pipeline latency** (mean, p95, p99) | 50 runs on `anonymized.png`, 3 warmup runs discarded | C++ Plaintext mean speedup >= **3x** over Python |
| **Per-node latency** (all 24 nodes) | Timing hooks in pipeline executor (Python: callback; C++: `steady_clock`) | Every C++ Plaintext node speedup >= **1.0x** (no node slower) |
| **Throughput** (images/sec) | Process 100 images; measure single-thread and multi-thread (8 cores) | C++ Plaintext multi-thread >= **20 img/s** |
| **Batch matching** (comparisons/sec) | 1-vs-1K, 1-vs-10K, 1-vs-1M with pre-generated templates | C++ Plaintext 1-vs-1M >= **1M comp/s**; C++ >= **50x** Python for 1-vs-1K |
| **Peak memory** (RSS) | `/usr/bin/time -v` or `getrusage(RUSAGE_SELF).ru_maxrss` | C++ Plaintext < **200MB**; C++ < **0.5x** Python RSS |
| **Startup time** | Process start to first pipeline invocation ready | C++ Plaintext (cached model) < **500ms** |

#### 17.2.2 Encryption Overhead Metrics (Informational, No Pass/Fail Gate)

These metrics quantify the cost of running C++ in its default encrypted mode. They do **not** gate merge — encryption adds inherent overhead. They are tracked for awareness and regression detection.

| Metric | How Measured | Expected Range |
|--------|-------------|---------------|
| **Matching latency overhead** | `encrypted_match_ms / plaintext_match_ms` | 500-2000x (single match: ~300ms encrypted vs ~0.3μs plaintext) |
| **Batch matching overhead** (1-vs-1K) | `encrypted_1vs1k_ms / plaintext_1vs1k_ms` | 50-200x (encrypted benefits from parallelism over ciphertexts) |
| **Template encryption time** | Time to encrypt one plaintext template → BFV ciphertext | < **200ms** |
| **Template decryption time** | Time to decrypt BFV ciphertext → plaintext template | < **50ms** |
| **Peak memory overhead** | `encrypted_rss_mb - plaintext_rss_mb` | ~50-100MB (FHE context + keys) |
| **Startup overhead** | `encrypted_startup_ms - plaintext_startup_ms` | < **2000ms** (FHE key generation) |
| **Pipeline latency (feature extraction)** | Identical in both modes (FHE only affects matching) | Overhead = **0ms** (verified) |

#### 17.2.3 Measurement Tools

**Python** (`biometric/iris/tools/bench_python_pipeline.py`):
```python
import time, statistics, json, resource
from iris import IRISPipeline, IRImage

def benchmark(image_path, n_warmup=3, n_runs=50):
    pipeline = IRISPipeline()
    img = load_image(image_path)
    for _ in range(n_warmup): pipeline(img)

    latencies = []
    for _ in range(n_runs):
        t0 = time.perf_counter_ns()
        pipeline(img)
        latencies.append((time.perf_counter_ns() - t0) / 1e6)

    return {
        "mean_ms": statistics.mean(latencies),
        "p95_ms": sorted(latencies)[int(0.95 * len(latencies))],
        "p99_ms": sorted(latencies)[int(0.99 * len(latencies))],
        "peak_rss_kb": resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    }
```

**C++** (`biometric/iris/tests/comparison/iris_comparison.cpp`):
Uses Google Benchmark with `--benchmark_format=json` output. Runs in both modes:
- `iris_comparison --no-encrypt --benchmark` → plaintext metrics
- `iris_comparison --benchmark` → encrypted metrics (default mode)

Also captures per-node timing via `PipelineExecutor` observer hooks and peak RSS via `getrusage`.

#### 17.2.4 Report Output (Performance Section)

```json
{
  "performance": {
    "pipeline_latency": {
      "python": {"mean_ms": 850, "p95_ms": 920, "p99_ms": 980},
      "cpp_plaintext_sequential": {"mean_ms": 180, "p95_ms": 195, "p99_ms": 210},
      "cpp_plaintext_parallel": {"mean_ms": 115, "p95_ms": 125, "p99_ms": 140},
      "cpp_encrypted_sequential": {"mean_ms": 180, "p95_ms": 195, "p99_ms": 210},
      "cpp_encrypted_parallel": {"mean_ms": 115, "p95_ms": 125, "p99_ms": 140},
      "note": "Pipeline latency identical: FHE only affects matching, not feature extraction",
      "speedup_plaintext_sequential": 4.72,
      "speedup_plaintext_parallel": 7.39,
      "pass": true
    },
    "matching_latency": {
      "python_single_match_us": 50,
      "cpp_plaintext_single_match_us": 0.3,
      "cpp_encrypted_single_match_ms": 300,
      "encryption_overhead_ratio": 1000000,
      "note": "Encrypted matching ~1M x slower per match, but amortized in batch via parallel ciphertext ops"
    },
    "batch_matching_comp_per_sec": {
      "python_1vs1k": 5000,
      "cpp_plaintext_1vs1k": 2000000,
      "cpp_encrypted_1vs1k": 100,
      "cpp_plaintext_1vs1m": 1500000,
      "encryption_overhead_batch_1vs1k": 20000,
      "pass": true,
      "pass_note": "Sign-off based on plaintext only"
    },
    "per_node_latency": [
      {"node": "segmentation", "python_ms": 193.2, "cpp_plaintext_ms": 48.1, "cpp_encrypted_ms": 48.1, "speedup": 4.02},
      {"node": "normalization", "python_ms": 12.5, "cpp_plaintext_ms": 0.8, "cpp_encrypted_ms": 0.8, "speedup": 15.6},
      {"node": "filter_bank", "python_ms": 85.0, "cpp_plaintext_ms": 3.2, "cpp_encrypted_ms": 3.2, "speedup": 26.6}
    ],
    "throughput_img_per_sec": {
      "python": 1.18, "cpp_plaintext_sequential": 5.6, "cpp_plaintext_parallel_8core": 25.0,
      "cpp_encrypted_sequential": 5.6, "cpp_encrypted_parallel_8core": 25.0,
      "note": "Throughput measures feature extraction only; matching benchmarked separately",
      "pass": true
    },
    "memory_peak_rss_mb": {
      "python": 450, "cpp_plaintext": 160, "cpp_encrypted": 220,
      "encryption_overhead_mb": 60,
      "pass": true,
      "pass_note": "Sign-off based on plaintext RSS"
    },
    "startup_ms": {
      "python": 2500, "cpp_plaintext": 350, "cpp_encrypted": 1800,
      "encryption_overhead_ms": 1450,
      "note": "Encrypted startup includes FHE key generation (one-time cost, keys cached after first run)",
      "pass": true,
      "pass_note": "Sign-off based on plaintext startup"
    },
    "encryption_overhead_summary": {
      "template_encrypt_ms": 150,
      "template_decrypt_ms": 30,
      "fhe_keygen_ms": 1200,
      "fhe_context_memory_mb": 50,
      "note": "Informational only — encryption overhead is expected and not gated"
    }
  }
}
```

### 17.3 Accuracy Comparison (Biometric + Numerical)

This is the most critical section. It validates that the C++ port produces **identical biometric decisions** to the Python reference.

**Sign-off mode**: All accuracy pass/fail thresholds in this section are evaluated on **C++ Plaintext vs Python**. FHE encryption does not touch the feature extraction pipeline, so iris codes, mask codes, and all intermediate outputs are identical in both C++ modes. The encrypted matching path is verified separately for **decision agreement** with the plaintext path (Section 17.3.7).

#### 17.3.1 Biometric Accuracy (FMR / FNMR / ROC / DET / EER)

Both implementations process the **same dataset** of iris images independently. For each, the framework:
1. Generates templates for all images
2. Computes all genuine (same-eye) and impostor (different-eye) Hamming distances
3. Sweeps the decision threshold from 0.0 to 0.5 to produce ROC and DET curves

**Metrics computed per implementation**:

| Metric | Definition | Python Baseline (from IRIS_PERFORMANCE_CARD.md) |
|--------|-----------|------------------------------------------------|
| **FMR @ threshold=0.35** | Fraction of impostor pairs with HD < 0.35 | < 4.1 x 10^-8 |
| **FNMR @ FMR=0.001** | Miss rate when FMR is held at 0.001 | 0.0012 |
| **FNMR @ FMR=0.0001** | Miss rate when FMR is held at 0.0001 | 0.0020 |
| **EER** | Threshold where FMR == FNMR | Empirical from dataset |
| **ROC curve** | TMR vs FMR at all thresholds | Plotted, visually identical |
| **DET curve** | FNMR vs FMR (log-log scale) | Plotted, visually identical |
| **FTE rate** | Failure-to-enroll (pipeline error/QA rejection) | 0.00238 |

**Pass thresholds**:

| Metric | Pass Condition |
|--------|---------------|
| FMR @ threshold=0.35 | C++ FMR <= Python FMR (equal or better) |
| FNMR @ FMR=0.001 | C++ FNMR <= Python FNMR + 0.001 (within 0.1% tolerance) |
| FNMR @ FMR=0.0001 | C++ FNMR <= Python FNMR + 0.001 |
| EER | abs(C++ EER - Python EER) < 0.001 |
| FTE rate | C++ FTE rate == Python FTE rate (same images fail/pass QA) |

**Why these should be identical**: If iris code bits are bit-exact (Section 17.3.3), then Hamming distances are identical (Section 17.3.4), which means all threshold-based metrics are mathematically guaranteed identical. Any deviation signals a bug.

**Test datasets**:
- **CI (every PR)**: `anonymized.png` only — generates 1 template, verifies pipeline runs without error. No genuine/impostor pairs (single image).
- **Standard (nightly/manual)**: CASIA1 (757 images, 108 subjects) — local `data/` symlink. 108 subjects x 7 images produces ~2,268 genuine pairs and ~270K+ impostor pairs, enough for robust ROC/DET.
- **Full (weekly)**: CASIA-Iris-Thousand (20,000 images, 1,000 subjects) — local `../eyed/data/Iris/CASIA-Iris-Thousand`. L+R eyes with 10 images each provides statistically robust FMR/FNMR baselines.

#### 17.3.2 Numerical Accuracy (Per-Node)

For each of the 24 pipeline nodes, run both implementations on identical input and compare outputs with statistical reporting.

**Tolerance table** (derived from Python e2e test patterns in `open-iris/tests/e2e_tests/utils.py` and MASTER_PLAN Section 11.2):

| Node / Output | Type | Tolerance | Source |
|---------------|------|-----------|--------|
| Segmentation predictions | float32 4xHxW | max element diff < **1e-4** | Phase 2 acceptance |
| Binary masks (pupil/iris/eyeball) | bool HxW | **exact match** | Integer output |
| Contour points (vectorization) | float32 Nx2 | Hausdorff distance < **1 pixel** | Phase 2 acceptance |
| Refined contour points | float32 Nx2 | Euclidean distance < **0.5 pixels** | Section 10.3 |
| Eye centers | float64 (x,y) | within **0.5 pixels** | Phase 3 acceptance |
| Ellipse fit parameters | float64 | < **0.1% relative error** | Phase 2 acceptance |
| Eye orientation | float64 | atol = **5e-5** (decimal=4) | Python `utils.py:41` |
| Offgaze score | float64 | atol = **5e-5** (decimal=4) | Python `utils.py:36` |
| Occlusion fractions | float64 | atol = **5e-5** (decimal=4) | Python `utils.py:43-51` |
| Sharpness score | float64 | within **1%** of Python | Phase 3 acceptance |
| Pupil/iris property ratios | float64 | atol = **5e-5** (decimal=4) | Python `utils.py:58-63` |
| Bounding box | float64 | atol = **5e-5** (decimal=4) | Python `utils.py:65-68` |
| Normalized iris image | float64 128x1024 | PSNR > **60dB** | Phase 3 acceptance |
| Normalized mask | bool 128x1024 | **exact match** | Boolean output |
| Gabor filter kernels | float64 complex | max diff < **1e-10** | Phase 4 acceptance |
| Filter response values | float64 complex | allclose(rtol=**1e-5**, atol=**1e-7**) | Python `test_e2e_conv_filter_bank.py:76` |
| Iris codes | bool (16,256,2) | **bit-exact** | Phase 4 acceptance |
| Mask codes | bool (16,256,2) | **bit-exact** | Phase 4 acceptance |
| Hamming distance | float64 | exact to **4 decimal places** | Python `test_e2e_hamming_distance_matcher.py:69` |

**Statistical report per node**:
```json
{
  "node": "normalization",
  "field": "normalized_image",
  "shape": [128, 1024],
  "dtype": "float64",
  "mean_abs_error": 1.2e-14,
  "max_abs_error": 3.5e-13,
  "p95_abs_error": 2.8e-14,
  "p99_abs_error": 1.1e-13,
  "rmse": 4.5e-14,
  "psnr_db": 280.5,
  "exact_match_fraction": 0.9998,
  "pass": true,
  "tolerance": "psnr > 60dB"
}
```

#### 17.3.3 Iris Code Bit Agreement

For the same input image processed by both implementations:

```
agreement_rate = 1.0 - popcount(cpp_code XOR python_code) / total_bits
```

Where `total_bits = 2 filters * 16 * 256 * 2 = 16,384 bits`.

**Threshold**: agreement_rate must be **1.0** (100% bit-exact). The encoding is deterministic (sign of real/imaginary Gabor response), so any disagreement is a bug.

#### 17.3.4 Hamming Distance Correlation

For N template pairs (N >= 190 from 20 images):
- Compute Hamming distances in both Python and C++ using identical matcher parameters (rotation_shift=15, normalise=true, norm_mean=0.45, norm_gradient=5e-5, separate_half_matching=true)
- Compute Pearson correlation coefficient

**Thresholds**:
| Metric | Pass Condition |
|--------|---------------|
| Pearson r | **== 1.0** |
| Max absolute discrepancy | **== 0.0** |

If any discrepancy exists, report which template pair and rotation shift caused it.

#### 17.3.5 Match/No-Match Decision Agreement

Confusion matrix at decision threshold = 0.35:

```
                    C++ Match    C++ No-Match
Python Match        TP (agree)   FN (disagree)
Python No-Match     FP (disagree) TN (agree)
```

**Threshold**: FN + FP must be **0** (perfect agreement).

#### 17.3.6 Template Cross-Compatibility

Test that templates generated by one implementation can be matched by the other:

| Test | Expected Result |
|------|----------------|
| C++ probe vs Python gallery (same image) | HD = 0.0 |
| Python probe vs C++ gallery (same image) | HD = 0.0 |
| C++ probe vs Python gallery (different images) | HD == within-language HD to 4 decimal places |
| Python probe vs C++ gallery (different images) | HD == within-language HD to 4 decimal places |

Template exchange format: `.npy` files (NumPy binary format, cross-language portable).

#### 17.3.7 Encrypted Matching Correctness (C++ Encrypted vs C++ Plaintext)

This verifies that FHE encryption does **not** alter match/no-match decisions. Since encrypted Hamming distance operates on BFV-packed bits and decrypts back to a scalar, any implementation bug (wrong slot packing, incorrect rotation, precision loss) would cause disagreement with plaintext matching.

**Tests**:

| Test | Method | Pass Condition |
|------|--------|---------------|
| **Decision agreement** | For N template pairs: compare `plaintext_match(A,B)` vs `decrypt(encrypted_match(encrypt(A), encrypt(B)))` at threshold=0.35 | **0 disagreements** |
| **HD value agreement** | Compare decrypted encrypted HD vs plaintext HD | max abs difference < **1e-6** (BFV integer arithmetic may round differently) |
| **Rotation agreement** | Encrypted matching with rotation_shift=15 selects the same optimal rotation as plaintext | **100% agreement** on selected rotation index |
| **Mask handling** | Encrypted mask AND + masked HD produces same fractional HD as plaintext | max abs difference < **1e-6** |
| **Batch encrypted vs batch plaintext** | 1-vs-100 encrypted gallery vs plaintext gallery: all 100 HDs match | max abs difference < **1e-6** |

**Why this test exists**: The encrypted path is the default production mode. If it silently disagrees with plaintext matching, the system would reject genuine users or accept impostors. This test catches:
- Incorrect bit packing into BFV slots
- Missing or wrong rotation handling under encryption
- Precision loss from BFV plaintext modulus being too small
- Mask handling bugs in the homomorphic circuit

**This test does NOT gate merge** on its own — it runs only after Phase 7 (OpenFHE integration). However, once Phase 7 is complete, encrypted correctness becomes a **merge-blocking** requirement.

### 17.4 Security Comparison

#### 17.4.1 Comparison Matrix

C++ encryption is **on by default**. Security tests run in both modes to verify the encrypted mode provides strictly stronger guarantees than plaintext.

| Dimension | Python (open-iris) | C++ Plaintext (`--no-encrypt`) | C++ Encrypted (default) | Verification Method |
|-----------|-------------------|-------------------------------|------------------------|-------------------|
| **Template exposure** | Plaintext numpy in memory; `gc.get_objects()` can extract | Plaintext `cv::Mat`; same exposure level as Python | **Encrypted** `Ciphertext`; bits indistinguishable from random | Entropy test on serialized template: encrypted > 7.99 bits/byte; grep for known iris code pattern in output (must not be found in encrypted mode) |
| **Memory safety** | GC-managed (no buffer overflows) | RAII, `std::span` bounds, no raw pointers | Same as plaintext + OpenFHE memory management | ASan/UBSan/MSan zero findings (both modes) |
| **Thread safety** | GIL prevents data races | Thread pool + `std::latch` barriers | Same (encrypted matching also uses thread pool) | TSan zero findings (both modes) |
| **Binary tamper resistance** | Source readable | Compiled binary, requires disassembly | Same + encrypted templates cannot be altered without keys | Qualitative assessment |
| **Input validation** | Pydantic, uncaught exceptions | `Result<T>`, never crashes | Same error handling for encrypt/decrypt failures | 10 malformed inputs → 0 crashes (both modes) |
| **Timing side-channels** | Non-constant time | Constant-time `std::popcount` | BFV operations are inherently constant-time (ciphertext size fixed) | Pearson(HD, time) < 0.05 for both plaintext and encrypted matching |
| **Template at rest** | Plaintext on disk (pickle/JSON) | Plaintext on disk (binary) | **Encrypted on disk** — default behavior | File entropy test; encrypted file cannot be decoded without private key |

#### 17.4.2 Automated Security Tests

Security tests run in **both C++ modes**. Pass/fail is determined per-mode:

```json
{
  "security": {
    "sanitizer_asan": "pass",
    "sanitizer_ubsan": "pass",
    "sanitizer_msan": "pass",
    "sanitizer_tsan": "pass",
    "timing_sidechannel": {
      "plaintext_matching_pearson": 0.02,
      "encrypted_matching_pearson": 0.008,
      "plaintext_pass": true,
      "encrypted_pass": true,
      "note": "Encrypted matching expected to have lower correlation (BFV ops are constant-time)"
    },
    "malformed_input_tests": 10,
    "malformed_input_crashes_plaintext": 0,
    "malformed_input_crashes_encrypted": 0,
    "malformed_input_pass": true,
    "fhe_template_entropy_bits_per_byte": 7.998,
    "fhe_plaintext_leak_detected": false,
    "fhe_pass": true,
    "template_at_rest": {
      "plaintext_file_entropy": 4.2,
      "encrypted_file_entropy": 7.998,
      "encrypted_file_decodable_without_key": false,
      "pass": true
    }
  }
}
```

### 17.5 Efficiency Comparison

#### 17.5.1 Metrics

**Sign-off**: Efficiency pass/fail is based on **C++ Plaintext vs Python**. Encrypted metrics are informational — encryption inherently trades space/time for security.

| Metric | Python | C++ Plaintext | C++ Encrypted (default) | Pass Threshold (Plaintext) | How Measured |
|--------|--------|--------------|------------------------|---------------------------|-------------|
| **Template size (memory)** | ~33 KB | ~4 KB (bit-packed) | ~1.6 MB (4 BFV ciphertexts) | C++ Plaintext < Python | `sys.getsizeof` / `sizeof` + heap |
| **Template size (disk)** | ~6 KB (base64 JSON) | ~4 KB (binary) | ~1.6 MB (serialized ciphertexts) | C++ Plaintext < Python | File size measurement |
| **Pipeline instance footprint** | ~400 MB | ~150 MB (ONNX + config + pool) | ~200 MB (+FHE keys) | C++ Plaintext < 0.5x Python | RSS delta at init |
| **CPU utilization** | ~1.0 (GIL) | > 3.0 (DAG-parallel) | Same (FHE only affects matching) | C++ > 2.0 | CPU time / wall time |
| **Cache miss ratio** | Higher (object headers) | Lower (contiguous `cv::Mat`) | Similar to plaintext | C++ < Python | `perf stat` (Linux container) |
| **Gallery storage (1M templates)** | ~6 GB (base64 JSON) | ~4 GB (binary) | ~1.6 TB (encrypted) | C++ Plaintext < Python | Extrapolated from single template |

#### 17.5.2 Encryption Storage Trade-off

Encryption expands template storage by ~400x. This is an inherent property of BFV ciphertexts, not a bug. The comparison framework reports this transparently:

| Storage Scenario | Plaintext C++ | Encrypted C++ | Overhead Factor |
|-----------------|---------------|---------------|----------------|
| 1 template | 4 KB | 1.6 MB | 400x |
| 1K gallery | 4 MB | 1.6 GB | 400x |
| 1M gallery | 4 GB | 1.6 TB | 400x |
| FHE public key | N/A | ~50 MB | One-time cost |
| FHE evaluation key | N/A | ~150 MB | One-time cost, needed for matching |

**Mitigation strategies** (documented, not compared):
- Ciphertext compression (CKKS packing, if switching schemes)
- Gallery sharding across encrypted partitions
- Hybrid: store plaintext in secure enclave, encrypt only for transit/external storage

#### 17.5.3 Report Output

```json
{
  "efficiency": {
    "template_size": {
      "python_memory_bytes": 33024,
      "cpp_plaintext_memory_bytes": 4096,
      "cpp_encrypted_memory_bytes": 1638400,
      "python_disk_bytes": 6200,
      "cpp_plaintext_disk_bytes": 4096,
      "cpp_encrypted_disk_bytes": 1638400,
      "plaintext_compression_ratio_vs_python": 8.07,
      "encryption_expansion_ratio": 400,
      "pass": true,
      "pass_note": "Sign-off based on plaintext efficiency only"
    },
    "pipeline_footprint_mb": {
      "python": 450,
      "cpp_plaintext": 160,
      "cpp_encrypted": 210,
      "encryption_overhead_mb": 50,
      "ratio_plaintext_vs_python": 0.36,
      "pass": true
    },
    "cpu_utilization": {
      "python_ratio": 1.0,
      "cpp_plaintext_sequential_ratio": 1.0,
      "cpp_plaintext_parallel_ratio": 3.33,
      "cpp_encrypted_parallel_ratio": 3.33,
      "note": "CPU utilization identical: FHE only affects matching, not pipeline parallelism"
    },
    "cache_miss_rate": {
      "python": 0.042,
      "cpp_plaintext": 0.018,
      "pass": true
    },
    "gallery_storage_1m_gb": {
      "python": 6.0,
      "cpp_plaintext": 4.0,
      "cpp_encrypted": 1600.0,
      "note": "Encrypted gallery 400x larger — inherent BFV property, not a regression"
    }
  }
}
```

### 17.6 Automated Test Harness

#### 17.6.1 Files

| File | Location | Purpose |
|------|----------|---------|
| `cross_language_comparison.py` | `biometric/iris/tools/` | Orchestrator: runs all 3 modes (Python, C++ plaintext, C++ encrypted), compares, reports |
| `generate_test_fixtures.py` | `biometric/iris/tools/` | Generates `.npy` + JSON fixtures from Python pipeline |
| `bench_python_pipeline.py` | `biometric/iris/tools/` | Python benchmarker (latency, throughput, memory) |
| `bench_python_per_node.py` | `biometric/iris/tools/` | Python per-node timing via pipeline callbacks |
| `iris_comparison.cpp` | `biometric/iris/tests/comparison/` | C++ comparison binary; supports `--no-encrypt` and default (encrypted) mode |
| `test_cross_language.cpp` | `biometric/iris/tests/comparison/` | GTest suite: loads Python .npy fixtures, compares vs C++ (both modes) |
| `test_encrypted_agreement.cpp` | `biometric/iris/tests/comparison/` | GTest suite: verifies encrypted matching agrees with plaintext matching |
| `npy_reader.hpp` | `biometric/iris/tests/comparison/` | Minimal .npy file reader for C++ (or use cnpy library) |
| `CMakeLists.txt` | `biometric/iris/tests/comparison/` | Build rules for comparison targets |
| `Dockerfile.comparison` | `biometric/iris/` | Container with both Python open-iris + C++ binary (built with OpenFHE) |
| `comparison.yml` | `.github/workflows/` | CI job: build, run harness in all 3 modes, check pass/fail, post PR comment |

The orchestrator runs the C++ binary **twice** per comparison:
1. `iris_comparison --no-encrypt ...` → plaintext results (used for sign-off vs Python)
2. `iris_comparison ...` → encrypted results (used for overhead reporting + correctness check vs plaintext)

#### 17.6.2 Fixture Format

All cross-language data uses `.npy` format (NOT pickle):
- NumPy's `.npy` is a simple binary format: magic bytes + shape/dtype header + raw array data
- C++ reads via cnpy library or a minimal custom reader (~100 lines)
- Scalar metadata (eye centers, offgaze, sharpness) saved as JSON

Fixture directory structure:
```
biometric/iris/tests/fixtures/
├── images/
│   ├── anonymized.png              # Copied from open-iris repo
│   └── casia1 -> ../../../../data  # Symlink to local CASIA1 dataset (108 subjects, 757 images)
├── python_reference/
│   └── anonymized/                 # Per-image directory
│       ├── segmentation_predictions.npy
│       ├── geometry_mask_pupil.npy
│       ├── geometry_mask_iris.npy
│       ├── normalized_image.npy
│       ├── normalized_mask.npy
│       ├── iris_response_0_real.npy
│       ├── iris_response_0_imag.npy
│       ├── iris_code_0.npy
│       ├── iris_code_1.npy
│       ├── mask_code_0.npy
│       ├── mask_code_1.npy
│       ├── metadata.json           # Scalars: eye_centers, offgaze, occlusion, sharpness, bbox
│       └── timings.json            # Per-node wall-clock times
└── comparison_history.jsonl        # Historical comparison reports (one JSON per line)
```

#### 17.6.3 JSON Report Schema (Complete)

The report separates **sign-off results** (C++ Plaintext vs Python, gates merge) from **encryption results** (informational + correctness check).

```json
{
  "report_version": "2.0",
  "timestamp": "2026-03-15T10:30:00Z",
  "git_commit": "abc123def456",
  "git_branch": "main",
  "test_images": ["anonymized.png"],
  "dataset_size": {"images": 1, "subjects": 1},
  "cpp_default_mode": "encrypted",

  "signoff": {
    "_comment": "C++ Plaintext vs Python — these gate merge",

    "performance": {
      "pipeline_latency": {
        "python": {"mean_ms": 850, "p95_ms": 920, "p99_ms": 980},
        "cpp_plaintext_sequential": {"mean_ms": 180, "p95_ms": 195, "p99_ms": 210},
        "cpp_plaintext_parallel": {"mean_ms": 115, "p95_ms": 125, "p99_ms": 140},
        "speedup_sequential": 4.72, "speedup_parallel": 7.39, "pass": true
      },
      "per_node_latency": [
        {"node": "segmentation", "python_ms": 193.2, "cpp_plaintext_ms": 48.1, "speedup": 4.02}
      ],
      "batch_matching": { "python_1vs1k": 5000, "cpp_plaintext_1vs1k": 2000000, "cpp_plaintext_1vs1m": 1500000, "pass": true },
      "memory_peak_rss_mb": { "python": 450, "cpp_plaintext": 160, "pass": true },
      "startup_ms": { "python": 2500, "cpp_plaintext": 350, "pass": true }
    },

    "accuracy": {
      "biometric": {
        "dataset": "CASIA1",
        "n_genuine_pairs": 100, "n_impostor_pairs": 4500,
        "python_fnmr_at_fmr_0001": 0.0012, "cpp_plaintext_fnmr_at_fmr_0001": 0.0012,
        "python_fnmr_at_fmr_00001": 0.0020, "cpp_plaintext_fnmr_at_fmr_00001": 0.0020,
        "python_eer": 0.0008, "cpp_plaintext_eer": 0.0008, "eer_delta": 0.0,
        "python_fte_rate": 0.00238, "cpp_plaintext_fte_rate": 0.00238,
        "roc_plot_path": "reports/roc_comparison.png",
        "det_plot_path": "reports/det_comparison.png",
        "pass": true
      },
      "numerical_per_node": [
        {"node": "segmentation", "field": "predictions", "max_abs_error": 0, "pass": true},
        {"node": "normalization", "field": "normalized_image", "psnr_db": 280, "pass": true}
      ],
      "iris_code_bit_agreement": { "total_bits": 16384, "matching_bits": 16384, "rate": 1.0, "pass": true },
      "hamming_distance_correlation": { "n_pairs": 190, "pearson_r": 1.0, "max_discrepancy": 0.0, "pass": true },
      "decision_agreement": { "threshold": 0.35, "agree": 190, "disagree": 0, "pass": true },
      "cross_compatibility": { "cpp_vs_python_same_image_hd": 0.0, "python_vs_cpp_same_image_hd": 0.0, "pass": true }
    },

    "efficiency": {
      "template_size": { "python_bytes": 33024, "cpp_plaintext_bytes": 4096, "pass": true },
      "pipeline_footprint_mb": { "python": 450, "cpp_plaintext": 160, "pass": true },
      "cpu_utilization": { "python": 1.0, "cpp_plaintext_parallel": 3.33, "pass": true }
    },

    "security": {
      "sanitizer_asan": "pass", "sanitizer_ubsan": "pass", "sanitizer_tsan": "pass",
      "timing_sidechannel_plaintext_pearson": 0.02, "timing_sidechannel_pass": true,
      "malformed_input_crashes": 0, "malformed_input_pass": true
    }
  },

  "encryption_report": {
    "_comment": "C++ Encrypted mode — informational + correctness check",

    "correctness": {
      "decision_agreement_vs_plaintext": { "n_pairs": 190, "agree": 190, "disagree": 0, "pass": true },
      "hd_value_agreement_vs_plaintext": { "max_abs_diff": 0.0, "pass": true },
      "rotation_agreement_vs_plaintext": { "agree": 190, "disagree": 0, "pass": true },
      "decision_agreement_failures": 0
    },

    "overhead": {
      "matching_latency": {
        "plaintext_single_match_us": 0.3, "encrypted_single_match_ms": 300,
        "overhead_ratio": 1000000
      },
      "batch_matching_1vs1k": {
        "plaintext_comp_per_sec": 2000000, "encrypted_comp_per_sec": 100,
        "overhead_ratio": 20000
      },
      "template_encrypt_ms": 150, "template_decrypt_ms": 30,
      "fhe_keygen_ms": 1200,
      "memory_overhead_mb": 60, "startup_overhead_ms": 1450
    },

    "storage": {
      "cpp_encrypted_template_bytes": 1638400,
      "cpp_encrypted_pipeline_footprint_mb": 210,
      "encryption_expansion_ratio": 400
    },

    "security_extras": {
      "fhe_entropy_bits_per_byte": 7.998,
      "fhe_plaintext_leak_detected": false,
      "timing_sidechannel_encrypted_pearson": 0.008,
      "template_at_rest_encrypted": true,
      "fhe_pass": true
    }
  },

  "signoff_pass": true,
  "signoff_failures": [],
  "encryption_correctness_pass": true,
  "overall_pass": true,
  "failures": []
}
```

**Pass/fail logic**:
- `signoff_pass`: ALL sign-off dimension checks pass (C++ Plaintext vs Python). **This gates merge.**
- `encryption_correctness_pass`: Encrypted matching agrees with plaintext matching (0 decision disagreements). **This gates merge after Phase 7.**
- `overall_pass`: `signoff_pass AND encryption_correctness_pass`. If OpenFHE is not yet integrated (Phases 1-6), `encryption_correctness_pass` is set to `true` (skipped).

#### 17.6.4 CI Integration

```yaml
# .github/workflows/comparison.yml
name: Cross-Language Comparison
on:
  pull_request:
    branches: [main]
  schedule:
    - cron: '0 2 * * 1'  # Weekly full dataset run

jobs:
  comparison:
    runs-on: ubuntu-latest
    container:
      image: ghcr.io/iriscpp/comparison:latest
    steps:
      - uses: actions/checkout@v4

      - name: Install Python open-iris
        run: cd open-iris && IRIS_ENV=SERVER pip install -e .

      - name: Build C++ comparison binary (with OpenFHE)
        run: |
          cd biometric/iris
          cmake -B build -DIRIS_BUILD_COMPARISON=ON -DIRIS_ENABLE_FHE=ON -DCMAKE_BUILD_TYPE=Release
          cmake --build build --target iris_comparison test_encrypted_agreement

      - name: Generate Python fixtures
        run: python biometric/iris/tools/generate_test_fixtures.py

      - name: Run comparison harness (all 3 modes)
        run: |
          python biometric/iris/tools/cross_language_comparison.py \
            --cpp-binary biometric/iris/build/tests/comparison/iris_comparison \
            --modes python,cpp_plaintext,cpp_encrypted \
            --output comparison_report.json

      - name: Check sign-off pass/fail (plaintext comparison only)
        run: |
          python -c "
          import json, sys
          r = json.load(open('comparison_report.json'))
          if not r['signoff_pass']:
              print('SIGN-OFF FAILURES (C++ Plaintext vs Python):')
              [print(f'  - {f}') for f in r['signoff_failures']]
              sys.exit(1)
          print('Sign-off checks passed (C++ Plaintext vs Python).')
          # Report encrypted mode results (informational)
          enc = r.get('encryption_report', {})
          if enc.get('decision_agreement_failures', 0) > 0:
              print(f'WARNING: {enc[\"decision_agreement_failures\"]} encrypted vs plaintext decision disagreements')
              sys.exit(1)
          print('Encrypted matching correctness verified.')
          "

      - name: Upload report
        uses: actions/upload-artifact@v4
        with:
          name: comparison-report
          path: comparison_report.json

      - name: Post PR summary comment
        if: github.event_name == 'pull_request'
        run: |
          python biometric/iris/tools/cross_language_comparison.py \
            --report comparison_report.json \
            --format github-comment \
            --output pr_comment.md
          gh pr comment ${{ github.event.pull_request.number }} --body-file pr_comment.md
        env:
          GH_TOKEN: ${{ secrets.GITHUB_TOKEN }}
```

#### 17.6.5 Regression Tracking

Each CI run appends the report to `comparison_history.jsonl` (one JSON object per line). The harness compares against the previous report:

| Regression Type | Detection | Action |
|----------------|-----------|--------|
| Performance regression | Any metric worsened by > 5% | Flag warning in PR comment |
| Accuracy regression | Any numerical tolerance that passed now fails | **Block merge** |
| Biometric regression | FNMR increased or FMR increased | **Block merge** |
| Security regression | Any sanitizer finding | **Block merge** |
| New failure | Any dimension that passed now shows `overall_pass: false` | **Block merge** |

### 17.7 Test Datasets

| Dataset | Images | Subjects | Usage | Access |
|---------|--------|----------|-------|--------|
| `anonymized.png` | 1 | 1 | **CI (every PR)**: pipeline runs, numerical accuracy, template cross-compatibility | In repo (from open-iris) |
| CASIA1 (local) | 757 | 108 | **Primary**: biometric accuracy (FMR/FNMR/ROC/DET), HD correlation, cross-language comparison | Local: `data/` symlink → `../eyed/data/Iris/CASIA1` (320x280 grayscale JPEGs, 7 images/subject, L+R eyes) |
| CASIA-Iris-Thousand (local) | 20,000 | 1,000 | **Full**: statistically robust FMR/FNMR baselines, large-scale batch matching benchmarks | Local: `../eyed/data/Iris/CASIA-Iris-Thousand` (640x480 grayscale JPEGs, 10 images/eye, L+R per subject) |
| MMU2 (local) | 995 | 100 | **Cross-dataset**: generalization test, different sensor/resolution (BMP format) | Local: `../eyed/data/Iris/MMU2` |

**Fixture generation**: `biometric/iris/tools/generate_test_fixtures.py` processes each image through the Python DEBUGGING_ENVIRONMENT pipeline, saves all 24 node outputs as `.npy` + scalar metadata as JSON.

**Sharing between containers**: Test images are mounted into the container from the local `data/` symlink (which points to `../eyed/data/Iris/CASIA1`). For larger datasets, mount additional volumes:
```yaml
volumes:
  - ../../eyed/data/Iris/CASIA1:/app/datasets/casia1:ro
  - ../../eyed/data/Iris/CASIA-Iris-Thousand:/app/datasets/casia-thousand:ro
  - ../../eyed/data/Iris/MMU2:/app/datasets/mmu2:ro
```

### 17.8 Implementation Sequencing

The comparison framework is built incrementally alongside the C++ phases. **Phases 1-6 run in plaintext mode only** (no encryption). **Phase 7 enables dual-mode testing.** Phase 8 completes the full harness.

| Phase | Mode | What Gets Compared | New Comparison Capability |
|-------|------|-------------------|--------------------------|
| **Phase 1** (Foundation) | Plaintext only | Encoder only | NPY reader, harness skeleton, encoder bit-exact test |
| **Phase 2** (Image Processing) | Plaintext only | Segmentation through geometry estimation | Per-node accuracy for 10 nodes, fixture generation |
| **Phase 3** (Eye Properties) | Plaintext only | All preprocessing nodes | Per-node accuracy for remaining nodes, PSNR test |
| **Phase 4** (Feature Extraction) | Plaintext only | Gabor, filter bank, encoder | Iris code bit agreement test, Gabor kernel exact match |
| **Phase 5** (Matching) | Plaintext only | Hamming distance, batch matcher | HD correlation, decision matrix, **biometric accuracy (FMR/FNMR/ROC/DET)** |
| **Phase 6** (Pipeline) | Plaintext only | Full pipeline end-to-end | Full performance comparison, per-node latency, throughput, template cross-compatibility |
| **Phase 7** (OpenFHE) | **Plaintext + Encrypted** | Encrypted templates & matching | Encrypted vs plaintext decision agreement (§17.3.7), template size/entropy, FHE overhead metrics, encrypted timing side-channel test |
| **Phase 8** (Hardening) | **Plaintext + Encrypted** | Everything | Complete 3-mode harness in CI, `Dockerfile.comparison`, all security tests both modes, regression tracking for both modes |

**Phase 7 marks the transition**: Before Phase 7, `encryption_correctness_pass` is automatically `true` (skipped). After Phase 7, encrypted correctness becomes a merge-blocking requirement. The CI workflow detects whether OpenFHE is integrated by checking if `iris_comparison` supports encrypted mode (`iris_comparison --check-fhe-support`).

**Default mode after Phase 7**: The C++ binary runs with encryption **on by default**. The comparison harness explicitly passes `--no-encrypt` for the plaintext sign-off run. This ensures the default production behavior (encrypted) is always tested.

---

## Appendix: Mathematical Reference

### A.1 Daugman Rubber-Sheet Normalization

For each output pixel at polar coordinates (r, θ) where r ∈ [0,1] and θ ∈ [0, 2π):

```
P(r, θ) = (1 - r) · P_pupil(θ) + r · P_iris(θ)
```

Where `P_pupil(θ)` and `P_iris(θ)` are boundary points at angle θ.

### A.2 2D Gabor Filter

```
G(x, y) = (1 / 2πσ_φσ_ρ) · exp(-½(x'²/σ_φ² + y'²/σ_ρ²)) · exp(j·2π·x'/λ_φ)

x' = x·cos(θ) - y·sin(θ)
y' = x·sin(θ) + y·cos(θ)
```

With DC correction: `G_corrected = G - mean(envelope) · carrier`

### A.3 Iris Code Encoding

For complex filter response z = a + bi at each position:
```
bit_0 = (a ≥ 0) ? 1 : 0    (real part sign)
bit_1 = (b ≥ 0) ? 1 : 0    (imaginary part sign)
```

### A.4 Hamming Distance

```
HD(A, B) = Σ((code_A ⊕ code_B) ∧ (mask_A ∧ mask_B)) / Σ(mask_A ∧ mask_B)
```

With rotation compensation:
```
HD_final = min over s ∈ [-S, S] of HD(A, rotate(B, s))
```

With normalization:
```
HD_norm = 0.5 - (0.5 - HD) · √(mask_count) · gradient
```

### A.5 BFV Encrypted Hamming Distance

Given iris code bits packed as integers (0 or 1) in BFV SIMD slots:
```
ct_diff = ct_A - ct_B                          // encrypted subtraction
ct_xor  = ct_diff * ct_diff                    // (a-b)² = a⊕b for binary
ct_mask = ct_mask_A * ct_mask_B                // encrypted AND
ct_masked_xor = ct_xor * ct_mask              // apply mask
ct_numerator = EvalSum(ct_masked_xor)          // sum all slots
ct_denominator = EvalSum(ct_mask)              // sum mask
// Decrypt both (requires secret key), compute HD = numerator / denominator
```

Multiplicative depth: 2 (one for xor, one for mask application).

---

*Last updated: 2026-02-28 — Phase 1+2 complete (106 tests)*
*Source: open-iris v1.11.0 by Worldcoin Foundation*
*Target: biometric/iris v2.0.0 (C++23) with OpenFHE integration*
