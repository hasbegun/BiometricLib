# ONNX Threading Problem Analysis

## Problem Statement

The C++ iris pipeline is 3.6x slower than Python (1183 ms vs 331 ms per image) on the CASIA1 benchmark. The root cause is that ONNX Runtime inference — which dominates ~85% of per-image cost — is hardcoded to a single thread in C++, while Python uses ONNX Runtime's default multi-threaded execution.

## Current Architecture

### ONNX Session (single-threaded)

```
File: src/nodes/onnx_segmentation.cpp:56
```

```cpp
ort_->options.SetIntraOpNumThreads(1);  // hardcoded
// InterOpNumThreads: not set (defaults to 1)
```

- `IntraOpNumThreads(1)` — parallelism *within* a single ONNX operator (e.g., a conv2d is split across cores). Set to 1: no parallelism.
- `InterOpNumThreads` — parallelism *across* independent ONNX operators in the graph. Not set: defaults to 1.
- Thread count is hardcoded, not exposed as a parameter.

### Pipeline Thread Pool (multi-threaded)

```
File: src/core/thread_pool.cpp:5-9
File: src/pipeline/pipeline_executor.cpp:59-107
```

- Pipeline creates a thread pool sized to `std::thread::hardware_concurrency()` (e.g., 12 on M4).
- DAG executor runs independent nodes within a level in parallel via the pool.
- Single-node levels (like segmentation, which is the first node) run sequentially on the calling thread.

### Comparison Tool (sequential images)

```
File: comparison/iris_comparison.cpp:146-267
```

- Processes images one at a time in a for-loop.
- Each image runs the full pipeline (with DAG parallelism) before moving to the next.
- No inter-image parallelism.

### Python ONNX (default threading)

```
File: open-iris/src/iris/nodes/segmentation/onnx_multilabel_segmentation.py:89
```

```python
session = ort.InferenceSession(model_path, providers=["CPUExecutionProvider"])
# No SessionOptions — uses ONNX Runtime defaults
```

- Python does **not** set `intra_op_num_threads` or `inter_op_num_threads`.
- ONNX Runtime default: uses all available cores for intra-op parallelism.
- On M4 with ~10-14 cores, ONNX parallelizes conv2d and other operators across all cores.

## Why Single Thread Was Chosen

The intent was to avoid thread contention between ONNX's internal threads and the pipeline executor's thread pool. If both create their own thread pools, the system becomes over-subscribed (e.g., 12 pipeline threads x N ONNX threads each = 12N threads competing for 12 cores).

## Why This Is a Problem

### 1. Segmentation is a single-node level

The segmentation node is the first pipeline stage and runs alone (no other nodes in its level). The DAG executor runs it on the calling thread, not via the pool. This means:

- The pipeline thread pool sits idle during ONNX inference.
- ONNX uses 1 thread while the other 11 cores do nothing.
- ~85% of per-image time is spent with 11/12 cores idle.

### 2. The comparison tool processes images sequentially

Since images are processed one-at-a-time, there is zero inter-image parallelism. The thread pool only helps with multi-node DAG levels (geometry + smoothing, etc.), which take ~15% of per-image time.

### 3. Over-subscription concern doesn't apply here

The contention argument only applies when multiple pipeline instances run concurrently on the same machine. For:
- **Benchmarking (comparison tool):** sequential, no contention — ONNX should use all cores.
- **Single-pipeline production:** one pipeline at a time — ONNX should use all cores.
- **Multi-pipeline production (e.g., 4 concurrent requests):** contention risk — ONNX should use fewer threads per instance.

## Proposed Solution

### Make ONNX thread count configurable

Add `intra_op_num_threads` as a parameter to `OnnxSegmentation`, defaulting to 0 (ONNX Runtime's "use all cores" behavior, matching Python).

#### Changes needed:

**1. `include/iris/nodes/onnx_segmentation.hpp` — add param**

```cpp
struct Params {
    std::string model_path;
    int input_width = 640;
    int input_height = 480;
    int input_channels = 1;
    bool denoise = true;
    int intra_op_num_threads = 0;  // 0 = use all cores (ONNX default)
};
```

**2. `src/nodes/onnx_segmentation.cpp` — use param**

```cpp
void OnnxSegmentation::init_session() {
    ort_ = std::make_shared<OrtState>();
    ort_->options.SetIntraOpNumThreads(params_.intra_op_num_threads);
    // ...
}
```

And in the constructor, parse the param:

```cpp
auto it = node_params.find("intra_op_num_threads");
if (it != node_params.end()) {
    params_.intra_op_num_threads = std::stoi(it->second);
}
```

**3. `pipeline.yaml` — optional override**

```yaml
- name: segmentation
  algorithm:
    class_name: iris.MultilabelSegmentation.create_from_hugging_face
    params:
      model_path: /src/models/iris_semseg_upp_scse_mobilenetv2.onnx
      denoise: true
      intra_op_num_threads: 0  # 0 = all cores
```

**4. `comparison/iris_comparison.cpp` — no changes needed**

The comparison tool creates an `IrisPipeline` which loads the YAML config. The thread count flows through the config automatically.

### Thread budget strategy for production

For deployments running multiple pipeline instances concurrently, the thread count should be tuned:

```
intra_op_num_threads = max(1, hardware_concurrency / expected_concurrent_pipelines)
```

For example, on a 12-core machine with 4 concurrent pipelines:
- `intra_op_num_threads = 3` (12 / 4 = 3 ONNX threads per pipeline)

This prevents over-subscription while still utilizing all cores.

## Impact Assessment

| Scenario | Current (1 thread) | Proposed (all cores) | Expected speedup |
|----------|--------------------|-----------------------|------------------|
| Single image, M4 (12 cores) | ~1000 ms ONNX | ~150-200 ms ONNX | ~5-7x |
| Full pipeline, single image | ~1183 ms | ~330-380 ms | ~3-3.5x |
| CASIA1 benchmark (743 images) | ~879 s | ~250-280 s | ~3-3.5x |

With multi-threaded ONNX, C++ should match or beat Python's 331 ms per image.

## Risk Assessment

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| ONNX thread pool + pipeline pool contention | Low (segmentation runs alone in its level) | Default to 0; allow override via config |
| Non-deterministic results from threading | None (ONNX produces identical output regardless of thread count) | N/A |
| Regression in existing tests | None (parameter is additive, default changes from 1 to 0) | Run full test suite after change |
| Docker container thread limits | Low (Docker inherits host thread count) | Test in Docker after change |

## Files to Modify

| File | Change |
|------|--------|
| `include/iris/nodes/onnx_segmentation.hpp` | Add `intra_op_num_threads` param (default 0) |
| `src/nodes/onnx_segmentation.cpp` | Parse param, use in `SetIntraOpNumThreads()` |
| `tests/unit/test_onnx_segmentation.cpp` | Add test for thread count param parsing |
| `comparison/COMPARISON_REPORT.md` | Update speed note after re-benchmark |

## Verification Plan

1. Build and run all 445 tests — must pass with no regression.
2. Re-run CASIA1 comparison with `intra_op_num_threads=0`.
3. Compare timing: expect ~3x speedup, matching or beating Python's 331 ms.
4. Verify biometric metrics unchanged (d', EER, bit agreement).
