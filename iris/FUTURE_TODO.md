# Future TODO — Out of Scope for Initial C++ Port

Items identified during master plan review that are important but deferred beyond the 8-phase implementation.

---

## Database & Gallery Management

- [ ] Database backend for encrypted template storage (candidates: RocksDB for embedded, PostgreSQL with bytea columns for server, or custom memory-mapped file store)
- [ ] Gallery indexing strategy — BFV ciphertexts cannot be indexed traditionally; investigate locality-sensitive hashing (LSH) on plaintext hash sketches before encryption, or partition-based sharding
- [ ] Template CRUD operations (create, read, update, delete) with transactional guarantees
- [ ] Gallery backup and recovery for encrypted templates
- [ ] Gallery compaction / garbage collection for deleted templates
- [ ] 1-vs-N search optimization — avoid linear scan over entire gallery (pre-filtering with iris code hash buckets)

## API & Integration

- [ ] REST API via cpp-httplib (endpoints: enroll, match, health, metrics)
- [ ] gRPC service definition (`.proto`) for high-performance client-server matching
- [ ] Client SDK / language bindings (Python ctypes/pybind11 wrapper for the C++ lib)
- [ ] CLI tool for batch enrollment, matching, and template management
- [ ] OpenAPI / Swagger spec for REST API

## Deployment & Operations

- [ ] Kubernetes deployment manifests (Helm chart)
- [ ] Prometheus metrics endpoint (pipeline latency histograms, match counts, error rates, FHE key age)
- [ ] Structured logging format (JSON for production, human-readable for dev)
- [ ] Distributed tracing (OpenTelemetry spans across pipeline DAG nodes)
- [ ] Health check with degraded-mode detection (model not loaded, FHE keys expired)
- [ ] Graceful shutdown with in-flight request draining
- [ ] Configuration precedence: YAML file < environment variables < CLI flags

## Key Management & PKI (Advanced)

> **Note**: Core key lifecycle (generation, storage, validation, expiry, re-encryption, secure memory) is now covered in MASTER_PLAN.md §6.6 and Phase 7 tasks 7.10–7.14. The items below are advanced operational concerns deferred beyond the initial port.

- [x] ~~FHE key rotation strategy (periodic re-encryption of gallery with new keys)~~ — basic `re_encrypt()` API now in §6.6.5; **automated** rotation policies remain deferred
- [x] ~~Key expiry and revocation~~ — expiry with TTL now in §6.6.5; **revocation lists** remain deferred
- [ ] HSM integration for private key storage (PKCS#11)
- [ ] Key distribution protocol for client-server deployments
- [ ] Separate key storage from template storage (defense in depth)
- [ ] Automated key rotation daemon / scheduled re-encryption
- [ ] Key revocation lists (CRL-style) for compromised keys

## GPU Acceleration

- [ ] ONNX Runtime CUDA/TensorRT execution provider for segmentation inference
- [ ] GPU-accelerated Hamming distance (CUDA popcount kernel for batch 1-vs-N)
- [ ] OpenCL fallback for non-NVIDIA GPUs
- [ ] Benchmark: CPU-only vs GPU for pipeline and matching separately

## Template Versioning & Migration

- [ ] Template version registry — what model + parameters produced each template
- [ ] Migration path when ONNX segmentation model is updated (re-enrollment vs compatibility layer)
- [ ] Backwards compatibility with Python open-iris v1 templates
- [ ] Template format version negotiation in client-server protocol

## Visualization & Debugging Tools

- [ ] C++ port of `IRISVisualizer` (plot IR images, contours, segmentation maps, normalized irises, iris codes)
- [ ] Pipeline debug mode with intermediate output dump (equivalent to Python DEBUGGING_ENVIRONMENT)
- [ ] Web-based template viewer for encrypted templates (shows metadata without decryption)

## Advanced Matching

- [ ] Score normalization across different sensors / capture conditions (CASIA1 vs CASIA-Thousand vs MMU2 calibration)
- [ ] Quality-weighted matching (use sharpness/occlusion scores to weight match confidence)
- [ ] Multi-template fusion at match time (combine multiple gallery templates per identity)
- [ ] Indexing with iris code hash for sub-linear 1-vs-N search

## Compliance & Audit

- [ ] License compliance audit for all dependencies (Apache 2.0, MIT, BSD, MPL2 composite analysis)
- [ ] Eigen MPL2 requirements verification for modifications
- [ ] GDPR considerations for biometric template storage (right to deletion, data minimization)
- [ ] Audit log for all template operations (who enrolled/matched/deleted, when)

## Input Sanitization

Image inputs (JPEG, PNG, BMP) can carry threat vectors beyond pixel data. Sanitize all inputs at the system boundary before they reach the pipeline.

- [ ] JPEG/PNG header validation — reject files with malformed headers, truncated streams, or unexpected trailing data
- [ ] Strip all metadata (EXIF, XMP, IPTC, ICC profiles, comments) — these can embed active content (JavaScript, XML entities, PHP payloads) that downstream viewers or log parsers may execute
- [ ] Enforce maximum image dimensions and file size before decoding (pre-decode check on header bytes, not just post-decode `cv::Mat` size)
- [ ] Decode-and-reencode sanitization — decode raw pixels with OpenCV, then re-encode to a clean buffer, discarding the original byte stream entirely; this neutralizes polyglot files (e.g., JPEG that is also valid JavaScript or HTML)
- [ ] Reject polyglot files — check magic bytes match declared format; reject files where JPEG/PNG magic is preceded by `<script`, `%PDF`, `PK` (ZIP), or other format signatures
- [ ] Validate pixel data post-decode — reject all-black, all-white, or zero-dimension images before pipeline entry
- [ ] Fuzz OpenCV decode path — run AFL/libFuzzer on `cv::imdecode` with crafted JPEG/PNG inputs to find crashes in the decode layer itself
- [ ] Content-Type enforcement at API boundary — if images arrive via REST/gRPC, validate Content-Type header matches actual file magic bytes
- [ ] Rate-limit and size-limit image uploads per client to prevent resource exhaustion via decompression bombs (e.g., 1x1 pixel JPEG header but 64000x64000 declared dimensions)

## TensorRT Backend

- [ ] Port `TensorRTMultilabelSegmentation` from Python (currently ONNX-only in C++ plan)
- [ ] `HostDeviceMem` helper for TensorRT memory management
- [ ] Benchmark TensorRT vs ONNX Runtime inference latency

---

*Last updated: 2026-02-28*
*These items will be prioritized after the initial 8-phase C++ port is complete and validated.*
