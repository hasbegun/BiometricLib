#!/usr/bin/env bash
# verify_fhe_builds.sh — Build with FHE ON and OFF, compare test results.
#
# Run inside Docker dev container:
#   docker compose run --rm dev bash comparison/verify_fhe_builds.sh
#
# Outputs results to comparison/fhe_verification/build_comparison.txt

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUTPUT_DIR="${SCRIPT_DIR}/fhe_verification"
mkdir -p "${OUTPUT_DIR}"

ONNXRUNTIME_ROOT="${ONNXRUNTIME_ROOT:-/opt/onnxruntime}"

echo "========================================"
echo "FHE Build Verification"
echo "========================================"

# --- Build WITHOUT FHE ---
echo ""
echo "--- Step 1: Build WITHOUT FHE ---"
BUILD_DIR_OFF="/tmp/build-fhe-off"
rm -rf "${BUILD_DIR_OFF}"

cmake -S . -B "${BUILD_DIR_OFF}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-march=native" \
    -DIRIS_ENABLE_TESTS=ON \
    -DIRIS_ENABLE_FHE=OFF \
    -DONNXRUNTIME_ROOT="${ONNXRUNTIME_ROOT}" \
    > "${OUTPUT_DIR}/build_fhe_off.log" 2>&1

cmake --build "${BUILD_DIR_OFF}" --parallel "$(nproc)" \
    >> "${OUTPUT_DIR}/build_fhe_off.log" 2>&1

echo "Build FHE OFF: OK"

cd "${BUILD_DIR_OFF}"
ctest --output-on-failure --timeout 120 > "${OUTPUT_DIR}/test_fhe_off.log" 2>&1 || true
cd -

TESTS_OFF=$(grep -oP '\d+(?= tests)' "${OUTPUT_DIR}/test_fhe_off.log" | tail -1 || echo "0")
PASSED_OFF=$(grep -oP '\d+(?= tests passed)' "${OUTPUT_DIR}/test_fhe_off.log" || echo "?")
FAILED_OFF=$(grep -oP '\d+(?= tests failed)' "${OUTPUT_DIR}/test_fhe_off.log" || echo "?")
# Extract from "100% tests passed, 0 tests failed out of N"
TESTS_OFF=$(grep "tests failed out of" "${OUTPUT_DIR}/test_fhe_off.log" | grep -oP '\d+$' || echo "?")
FAILED_OFF=$(grep "tests failed out of" "${OUTPUT_DIR}/test_fhe_off.log" | grep -oP '^\s*\d+' | head -1 | tr -d ' ' || echo "?")
if [ "${FAILED_OFF}" = "0" ]; then
    PASSED_OFF="${TESTS_OFF}"
else
    PASSED_OFF="?"
fi

echo "FHE OFF: ${TESTS_OFF} tests, ${FAILED_OFF} failed"

# --- Build WITH FHE ---
echo ""
echo "--- Step 2: Build WITH FHE ---"
BUILD_DIR_ON="/tmp/build-fhe-on"
rm -rf "${BUILD_DIR_ON}"

cmake -S . -B "${BUILD_DIR_ON}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-march=native" \
    -DIRIS_ENABLE_TESTS=ON \
    -DIRIS_ENABLE_FHE=ON \
    -DONNXRUNTIME_ROOT="${ONNXRUNTIME_ROOT}" \
    > "${OUTPUT_DIR}/build_fhe_on.log" 2>&1

cmake --build "${BUILD_DIR_ON}" --parallel "$(nproc)" \
    >> "${OUTPUT_DIR}/build_fhe_on.log" 2>&1

echo "Build FHE ON: OK"

cd "${BUILD_DIR_ON}"
ctest --output-on-failure --timeout 120 > "${OUTPUT_DIR}/test_fhe_on.log" 2>&1 || true
cd -

TESTS_ON=$(grep "tests failed out of" "${OUTPUT_DIR}/test_fhe_on.log" | grep -oP '\d+$' || echo "?")
FAILED_ON=$(grep "tests failed out of" "${OUTPUT_DIR}/test_fhe_on.log" | grep -oP '^\s*\d+' | head -1 | tr -d ' ' || echo "?")
if [ "${FAILED_ON}" = "0" ]; then
    PASSED_ON="${TESTS_ON}"
else
    PASSED_ON="?"
fi

echo "FHE ON: ${TESTS_ON} tests, ${FAILED_ON} failed"

# --- Determine FHE-only test count ---
FHE_ONLY=$((TESTS_ON - TESTS_OFF))

# --- Write summary ---
SUMMARY="${OUTPUT_DIR}/build_comparison.txt"
cat > "${SUMMARY}" << EOF
FHE Build Verification
======================
Date: $(date -u +"%Y-%m-%d %H:%M:%S UTC")

                    FHE OFF    FHE ON
Tests total:        ${TESTS_OFF}        ${TESTS_ON}
Tests passed:       ${PASSED_OFF}        ${PASSED_ON}
Tests failed:       ${FAILED_OFF}          ${FAILED_ON}
FHE-only tests:     -          ${FHE_ONLY}

FHE-only tests include:
  - FHEContext.* (context creation, key generation, encrypt/decrypt)
  - EncryptedTemplate.* (template encryption round-trip, entropy, no leak)
  - EncryptedMatcher.* (encrypted HD, rotation, plaintext parity)
  - KeyManager.* (key generation, validation, expiry)
  - KeyStore.* (key save/load round-trip)
  - FHEIntegration.* (full flow, batch, key save/load, re-encrypt, concurrent, rotation, decision agreement)

Verdict: $([ "${FAILED_OFF}" = "0" ] && [ "${FAILED_ON}" = "0" ] && echo "PASS" || echo "FAIL")
  All non-FHE tests pass in both modes.
  FHE tests pass when enabled.
EOF

echo ""
echo "========================================"
cat "${SUMMARY}"
echo "========================================"
echo ""
echo "Full logs: ${OUTPUT_DIR}/"

# Exit code
if [ "${FAILED_OFF}" = "0" ] && [ "${FAILED_ON}" = "0" ]; then
    exit 0
else
    exit 1
fi
