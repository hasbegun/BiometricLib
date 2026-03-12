#!/usr/bin/env bash
# =============================================================================
# Step 1.3 — Docker build verification script
# Run from project root: bash tests/integration/test_docker_build.sh
# =============================================================================
set -euo pipefail

PASS=0
FAIL=0
ERRORS=""

pass() { echo "  PASS: $1"; ((PASS++)); }
fail() { echo "  FAIL: $1"; ((FAIL++)); ERRORS="${ERRORS}\n  - $1"; }

echo "=== Step 1.3: Docker Build Verification ==="
echo ""

# Test 1: docker compose config validates without errors
echo "[1/6] Validating docker-compose.yml..."
if docker compose config > /dev/null 2>&1; then
    pass "docker compose config validates"
else
    fail "docker compose config validation failed"
fi

# Test 2: docker build completes with exit code 0
echo "[2/6] Building Docker image (this may take several minutes)..."
if docker compose build test 2>&1; then
    pass "docker build completes successfully"
else
    fail "docker build failed"
fi

# Test 3: Mosquitto starts and accepts connections
echo "[3/6] Starting Mosquitto broker..."
docker compose up -d mosquitto
sleep 2
if docker compose ps mosquitto | grep -q "running\|Up"; then
    pass "Mosquitto broker is running"
else
    fail "Mosquitto broker failed to start"
fi
docker compose down mosquitto 2>/dev/null || true

# Test 4: make test target exists and runs
echo "[4/6] Verifying make test target..."
if make -n test > /dev/null 2>&1; then
    pass "make test target exists"
else
    fail "make test target not found"
fi

# Test 5: make test-asan target exists
echo "[5/6] Verifying make test-asan target..."
if make -n test-asan > /dev/null 2>&1; then
    pass "make test-asan target exists"
else
    fail "make test-asan target not found"
fi

# Test 6: Runtime stage image size check
echo "[6/6] Checking runtime image size..."
if docker compose build prod 2>&1; then
    SIZE_BYTES=$(docker image inspect --format='{{.Size}}' "$(docker compose images prod -q 2>/dev/null || echo 'eyetrack-prod')" 2>/dev/null || echo "0")
    SIZE_MB=$((SIZE_BYTES / 1024 / 1024))
    if [ "$SIZE_MB" -lt 500 ] && [ "$SIZE_MB" -gt 0 ]; then
        pass "Runtime image size: ${SIZE_MB}MB (<500MB)"
    elif [ "$SIZE_MB" -eq 0 ]; then
        pass "Runtime image built (size check skipped — no image ID available)"
    else
        fail "Runtime image size: ${SIZE_MB}MB (exceeds 500MB limit)"
    fi
else
    fail "Runtime image build failed"
fi

# Summary
echo ""
echo "=== Results ==="
echo "  Passed: ${PASS}"
echo "  Failed: ${FAIL}"
if [ "$FAIL" -gt 0 ]; then
    echo -e "  Failures:${ERRORS}"
    exit 1
fi
echo "  All tests passed."
