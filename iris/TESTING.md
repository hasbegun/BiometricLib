# Testing Guide

How to build, run, and extend the iris C++ test suite.

All commands run from `biometric/iris/`.

---

## Quick Start

```bash
make test        # Build everything + run all 445 tests in Docker (Linux aarch64)
```

That's it. If the output ends with `100% tests passed, 0 tests failed out of 445`, everything works.

---

## Make Targets

| Command        | What it does                                         |
|----------------|------------------------------------------------------|
| `make test`    | Build + run all unit tests inside Docker             |
| `make rebuild` | Full clean rebuild (no Docker cache)                 |
| `make dev`     | Start interactive dev container with source mounted  |
| `make bench`   | Build + run benchmarks                               |
| `make lint`    | Check clang-format compliance                        |
| `make count`   | Show test count and lines of code                    |
| `make test-asan` | Run tests under ASan + UBSan                       |
| `make test-tsan` | Run tests under ThreadSanitizer                    |
| `make clean`   | Remove Docker images and build artifacts             |
| `make help`    | Show all targets                                     |

---

## Running Specific Tests

### Inside the Dev Container

```bash
make dev
# You are now inside the container at /src

# Configure (creates build/ directory — required on first run)
cmake -S . -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-march=native" \
    -DIRIS_ENABLE_TESTS=ON \
    -DIRIS_ENABLE_BENCH=ON \
    -DIRIS_ENABLE_FHE=ON \
    -DONNXRUNTIME_ROOT=/opt/onnxruntime

# Build everything
cmake --build build --parallel $(nproc)

# Run all tests
cd build && ctest --output-on-failure

# Run a specific test binary
./tests/test_hamming_distance_matcher

# Run a specific test case
./tests/test_hamming_distance_matcher --gtest_filter="SimpleHammingDistanceMatcher.IdenticalZero"

# Run all tests matching a pattern
./tests/test_hamming_distance_matcher --gtest_filter="SimpleHammingDistanceMatcher.*"

# List all tests in a binary without running them
./tests/test_hamming_distance_matcher --gtest_list_tests

# Run with ctest by name pattern
ctest -R "BatchMatcher"           # All tests with "BatchMatcher" in name
ctest -R "TemplateAlignment"      # All template alignment tests
ctest -R "Phase5"                 # Won't match — use the actual test names

# Verbose output for all tests
ctest -V

# Run tests in parallel (N = number of parallel jobs)
ctest -j$(nproc)
```

### From Outside Docker (One-Off)

```bash
# Run a single test binary via docker compose
docker compose run --rm test bash -c \
  "cmake --build build --parallel \$(nproc) && \
   ./build/tests/test_hamming_distance_matcher"
```

---

## Test Organisation

```
tests/
├── unit/                     # 422 unit tests across 62 files
│   ├── test_types.cpp                   # Core types, PackedIrisCode
│   ├── test_error.cpp                   # Error handling
│   ├── test_cancellation.cpp            # CancellationToken
│   ├── test_thread_pool.cpp             # ThreadPool parallel primitives
│   ├── test_async_io.cpp                # Async file I/O
│   ├── test_simd_popcount.cpp           # SIMD popcount correctness
│   ├── test_encoder.cpp                 # Iris code binarization
│   ├── test_node_registry.cpp           # Node auto-registration
│   │
│   │  # Phase 2: Image Processing
│   ├── test_geometry_utils.cpp          # Polar/Cartesian, area, diameter
│   ├── test_specular_reflection.cpp     # Specular highlight detection
│   ├── test_binarization.cpp            # Segmentation binarization
│   ├── test_vectorization.cpp           # Contour extraction
│   ├── test_geometry_refinement.cpp     # Interpolation + distance filter
│   ├── test_eye_centers.cpp             # Bisectors eye center
│   ├── test_eye_orientation.cpp         # Moment-of-area orientation
│   ├── test_geometry_estimation.cpp     # Smoothing, extrapolation, fitting
│   ├── test_segmentation.cpp            # ONNX stub error paths
│   │
│   │  # Phase 3: Eye Properties & Normalization
│   ├── test_normalization_utils.cpp     # Shared normalisation helpers
│   ├── test_offgaze_estimation.cpp      # Eccentricity offgaze
│   ├── test_pupil_iris_property.cpp     # Pupil/iris ratio
│   ├── test_iris_bbox.cpp               # Bounding box
│   ├── test_noise_mask_union.cpp        # Noise mask OR
│   ├── test_occlusion_calculator.cpp    # Eye occlusion
│   ├── test_linear_normalization.cpp    # Daugman rubber-sheet
│   ├── test_nonlinear_normalization.cpp # Quadratic radial grids
│   ├── test_perspective_normalization.cpp # Perspective warp
│   ├── test_sharpness_estimation.cpp    # Laplacian sharpness
│   ├── test_object_validators.cpp       # 7 single-object validators
│   ├── test_cross_object_validators.cpp # 2 cross-object validators
│   │
│   │  # Phase 4: Feature Extraction
│   ├── test_gabor_filter.cpp            # Gabor kernel generation
│   ├── test_log_gabor_filter.cpp        # Log-Gabor freq-domain kernel
│   ├── test_regular_probe_schema.cpp    # Probe grid sampling
│   ├── test_conv_filter_bank.cpp        # Spatial convolution
│   ├── test_fragile_bit_refinement.cpp  # Fragile bit thresholding
│   ├── test_feature_extraction_chain.cpp # Full extraction chain
│   │
│   │  # Phase 5: Matching & Template Operations
│   ├── test_hamming_distance_matcher.cpp # Simple + full HD matcher
│   ├── test_batch_matcher.cpp           # 1-vs-N, N-vs-N matching
│   ├── test_template_alignment.cpp      # Template rotation alignment
│   ├── test_majority_vote_aggregation.cpp # Bit voting aggregation
│   ├── test_template_identity_filter.cpp # Outlier filtering
│   │
│   │  # Phase 6: Pipeline Orchestration
│   ├── test_pipeline_context.cpp        # PipelineContext typed get/set + move-only
│   ├── test_node_dispatcher.cpp         # Type dispatch table
│   ├── test_callback_dispatcher.cpp     # Validator callback dispatch
│   ├── test_pipeline_dag.cpp            # DAG builder + validation + YAML load
│   ├── test_pipeline_executor.cpp       # Level-by-level execution
│   ├── test_iris_pipeline.cpp           # IrisPipeline facade
│   ├── test_multiframe_pipeline.cpp     # Aggregation + multiframe
│   │
│   │  # Deferred Items (filled after Phase 6)
│   ├── test_base64.cpp                  # Base64 encode/decode (RFC 4648)
│   ├── test_template_serializer.cpp     # Binary + Python-format serialization
│   ├── test_npy_writer.cpp              # NumPy .npy file writer
│   ├── test_secure_buffer.cpp           # SecureBuffer (mlock + explicit_bzero)
│   ├── test_template_npy.cpp            # PackedIrisCode NPY round-trip
│   │
│   │  # Phase 7: FHE (conditional on IRIS_ENABLE_FHE)
│   ├── test_fhe_context.cpp             # FHE context + key generation
│   ├── test_encrypted_template.cpp      # Encrypt/decrypt/re-encrypt templates
│   ├── test_encrypted_matcher.cpp       # Encrypted HD + rotation matching
│   ├── test_key_manager.cpp             # Key generation + metadata + expiry
│   ├── test_key_store.cpp               # Key save/load/permissions
│   │
│   │  # Phase 8: Hardening
│   └── test_api_hardening.cpp           # API boundary edge cases + fuzz-lite
│
├── integration/              # 15 integration tests
│   ├── test_pipeline_integration.cpp    # Post-seg chain, parity, concurrency
│   └── test_fhe_integration.cpp         # Full FHE flow, batch, key lifecycle
├── bench/                    # Performance benchmarks
│   ├── bench_matcher.cpp                # Plaintext matching benchmarks
│   └── bench_crypto.cpp                 # FHE key gen, encryption, matching
├── e2e/                      # End-to-end tests (future)
├── comparison/               # Cross-language C++/Python tests (future)
│   ├── npy_reader.hpp                   # NumPy .npy file reader
│   ├── npy_writer.hpp                   # NumPy .npy file writer
│   └── template_npy_utils.hpp           # PackedIrisCode NPY I/O wrapper
├── fixtures/                 # Test data files (future)
└── CMakeLists.txt
```

---

## Test Counts by Phase

| Phase | Description                      | Tests | Cumulative |
|-------|----------------------------------|------:|----------:|
| 1     | Foundation                       |    50 |        50 |
| 2     | Image Processing Nodes           |    56 |       106 |
| 3     | Eye Properties & Normalization   |    80 |       186 |
| 4     | Feature Extraction & Encoding    |    39 |       225 |
| 5     | Matching & Template Operations   |    57 |       282 |
| 6     | Pipeline Orchestration           |    67 |       349 |
| —     | Deferred Items (base64, serializer, npy writer, secure buffer, callback test) | 23 | 372 |
| 7     | OpenFHE Integration (FHE context, encrypted template/matcher, keys) | 45 | 417 |
| 8     | Hardening (LogGabor, API hardening, move-only ctx, re-encrypt, concurrency, NPY utils) | 25 | 442 |
| —     | Sign-off tests (entropy, plaintext leak, decision agreement) | 3 | 445 |

---

## Build Configurations

The project supports several CMake presets (defined in `CMakePresets.json`):

| Preset                    | Use case                              |
|---------------------------|---------------------------------------|
| `linux-release`           | Default: optimised, all tests + bench (Linux aarch64) |
| `linux-debug-sanitizers`  | Debug + AddressSanitizer + UBSan      |
| `linux-debug-tsan`        | Debug + ThreadSanitizer               |
| `linux-aarch64`           | Explicit ARM64 flags (`-march=armv8.2-a+crypto`) |

**Target platform**: Linux aarch64 (ARM64) only. Development happens on macOS Apple Silicon via Docker.

Docker uses `linux-release` by default. To test with sanitizers inside the dev container:

```bash
make dev
cmake --preset linux-debug-sanitizers
cmake --build build/debug-san --parallel $(nproc)
cd build/debug-san && ctest --output-on-failure
```

---

## Compiler Warnings

All builds use strict warnings treated as errors:

```
-Wall -Wextra -Wpedantic -Werror
-Wconversion -Wsign-conversion -Wdouble-promotion
-Wnull-dereference -Wformat=2
```

Any new code must compile clean under these flags.

OpenFHE headers trigger warnings under these strict flags. All crypto source files use per-file
`-Wno-null-dereference` (set via `set_source_files_properties` in CMakeLists.txt) and test/bench
files wrapping OpenFHE includes use `#pragma GCC diagnostic push/pop` blocks to suppress:
`-Wconversion`, `-Wsign-conversion`, `-Wdouble-promotion`, `-Wnull-dereference`, `-Wpedantic`,
`-Wold-style-cast`, and GCC-only `-Wuseless-cast`.

---

## FHE Tests

FHE tests are conditional on `-DIRIS_ENABLE_FHE=ON` and guarded by `#ifdef IRIS_HAS_FHE`.
When FHE is disabled, these tests are simply not compiled.

```bash
# Run only FHE tests (inside dev container)
cd build && ctest -R "fhe|encrypted|key_" --output-on-failure

# Run a specific FHE test binary
./tests/test_encrypted_matcher --gtest_filter="EncryptedMatcher.*"
```

FHE tests are slower than plaintext tests due to key generation and homomorphic operations.
Each FHE test fixture creates its own `FHEContext` and generates keys (~2-3s per context).
The integration test `test_fhe_integration.cpp` covers the full end-to-end flow including
key save/load cycles and rotation matching.

---

## Sanitizer Testing

Run the full test suite under memory and thread sanitizers via dedicated Docker services:

```bash
make test-asan   # ASan + UBSan: detects buffer overflows, use-after-free, UB
make test-tsan   # ThreadSanitizer: detects data races
```

These rebuild from scratch with `-DCMAKE_BUILD_TYPE=Debug` and the respective sanitizer flags.
They are slower than the release build but catch subtle bugs that release mode optimises away.

---

## Adding New Tests

1. Create `tests/unit/test_your_node.cpp`:
   ```cpp
   #include <iris/nodes/your_node.hpp>
   #include <gtest/gtest.h>

   using namespace iris;

   TEST(YourNode, BasicBehavior) {
       YourNode node;
       auto result = node.run(input);
       ASSERT_TRUE(result.has_value());
       EXPECT_EQ(result.value().field, expected);
   }
   ```

2. Add to `tests/CMakeLists.txt`:
   ```cmake
   iris_add_test(test_your_node unit/test_your_node.cpp)
   ```

3. Run: `make test`

---

## Troubleshooting

**`/src/build is not a directory` in dev container:**

The dev container mounts your local source over `/src`, so the `build/` directory from the Docker image isn't available. Run the configure step first:

```bash
make dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-march=native" -DIRIS_ENABLE_TESTS=ON \
    -DIRIS_ENABLE_BENCH=ON -DIRIS_ENABLE_FHE=ON \
    -DONNXRUNTIME_ROOT=/opt/onnxruntime
cmake --build build --parallel $(nproc)
```

**Build fails with Docker cache issues:**
```bash
make rebuild    # Full rebuild without Docker cache
```

**Need to inspect build errors interactively:**
```bash
make dev
# Configure first if build/ doesn't exist (see above)
cmake --build build --parallel $(nproc) 2>&1 | head -50
```

**Test binary crashes without useful output:**
```bash
make dev
cd build
./tests/test_failing_binary --gtest_filter="FailingTest" 2>&1
```

**Want to see what ctest would run:**
```bash
make dev
cd build && ctest -N    # Lists tests without running
```
