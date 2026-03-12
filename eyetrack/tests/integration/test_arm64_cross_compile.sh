#!/usr/bin/env bash
# test_arm64_cross_compile.sh — ARM64 cross-compilation smoke test
#
# Verifies that core eyetrack source files compile for aarch64 using
# the cross-compiler. Only compiles to object files (no linking) since
# aarch64 libraries (OpenCV, etc.) are not installed.
#
# Requirements: aarch64-linux-gnu-g++ installed

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

CC=aarch64-linux-gnu-g++
PASS=0
FAIL=0

pass() { echo "  PASS: $1"; PASS=$((PASS + 1)); }
fail() { echo "  FAIL: $1"; FAIL=$((FAIL + 1)); }

echo "=== ARM64 Cross-Compilation Smoke Test ==="
echo "Project root: $PROJECT_ROOT"
echo ""

# --- Test 1: cross-compiler exists ---
echo "Test 1: aarch64-linux-gnu-g++ available"
if command -v "$CC" &>/dev/null; then
    pass "cross-compiler found: $($CC --version | head -1)"
else
    fail "aarch64-linux-gnu-g++ not found"
    echo "Install with: apt-get install g++-aarch64-linux-gnu"
    exit 1
fi

# --- Test 2: compile core source files to aarch64 objects ---
INCLUDE_DIRS=(
    "-I${PROJECT_ROOT}/include"
    "-isystem" "/usr/include/opencv4"
    "-isystem" "/usr/include/eigen3"
)

CXX_FLAGS="-std=c++23 -c -march=armv8.2-a -Wall -Wextra"

OUTDIR=$(mktemp -d)
trap "rm -rf $OUTDIR" EXIT

CORE_SOURCES=(
    "src/core/types.cpp"
    "src/core/error.cpp"
    "src/core/config.cpp"
    "src/core/node_registry.cpp"
    "src/core/thread_pool.cpp"
    "src/utils/geometry_utils.cpp"
)

echo ""
echo "Test 2: compile core sources to aarch64 object files"
ALL_COMPILED=true
for src in "${CORE_SOURCES[@]}"; do
    base=$(basename "$src" .cpp)
    echo -n "  Compiling $src ... "
    if $CC $CXX_FLAGS "${INCLUDE_DIRS[@]}" \
        "$PROJECT_ROOT/$src" -o "$OUTDIR/${base}.o" 2>&1; then
        echo "OK"
    else
        echo "FAILED"
        ALL_COMPILED=false
    fi
done

if $ALL_COMPILED; then
    pass "all ${#CORE_SOURCES[@]} core sources compiled for aarch64"
else
    fail "some sources failed to compile"
fi

# --- Test 3: verify object files are aarch64 architecture ---
echo ""
echo "Test 3: verify object file architecture"
ALL_AARCH64=true
for src in "${CORE_SOURCES[@]}"; do
    base=$(basename "$src" .cpp)
    obj="$OUTDIR/${base}.o"
    if [ -f "$obj" ]; then
        ARCH_INFO=$(file "$obj")
        if echo "$ARCH_INFO" | grep -qi "aarch64\|ARM aarch64"; then
            echo "  $base.o: aarch64 confirmed"
        else
            echo "  $base.o: WRONG ARCH: $ARCH_INFO"
            ALL_AARCH64=false
        fi
    fi
done

if $ALL_AARCH64; then
    pass "all object files are aarch64 architecture"
else
    fail "some object files have wrong architecture"
fi

# --- Test 4: objects do not contain x86_64 instructions ---
echo ""
echo "Test 4: no x86_64 architecture in object files"
HAS_X86=false
for src in "${CORE_SOURCES[@]}"; do
    base=$(basename "$src" .cpp)
    obj="$OUTDIR/${base}.o"
    if [ -f "$obj" ] && file "$obj" | grep -qi "x86-64\|x86_64"; then
        echo "  $base.o: contains x86_64!"
        HAS_X86=true
    fi
done

if ! $HAS_X86; then
    pass "no x86_64 architecture detected"
else
    fail "x86_64 architecture found in aarch64 objects"
fi

# --- Summary ---
echo ""
echo "=== Results: $PASS passed, $FAIL failed ==="
[ "$FAIL" -eq 0 ] && exit 0 || exit 1
