#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
EYETRACK_ROOT="$SCRIPT_DIR/../../"
BUILD_DIR="$EYETRACK_ROOT/build-ffi"
FRAMEWORKS_DIR="$SCRIPT_DIR/../macos/Runner/Frameworks"
MODEL_DIR="$SCRIPT_DIR/../macos/Runner/Resources/models"

echo "=== Building eyetrack FFI dylib ==="
echo "Source: $EYETRACK_ROOT"
echo "Build:  $BUILD_DIR"

# Find Homebrew OpenCV cmake dir
OPENCV_DIR="$(dirname "$(find /opt/homebrew -name 'OpenCVConfig.cmake' 2>/dev/null | head -1)")"
if [ -z "$OPENCV_DIR" ]; then
    echo "ERROR: OpenCV not found. Install with: brew install opencv"
    exit 1
fi

cmake -B "$BUILD_DIR" -S "$EYETRACK_ROOT" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=/opt/homebrew \
    -DOpenCV_DIR="$OPENCV_DIR" \
    -DEYETRACK_BUILD_FFI=ON \
    -DEYETRACK_ENABLE_TESTS=OFF \
    -DEYETRACK_ENABLE_GRPC=OFF \
    -DEYETRACK_ENABLE_MQTT=OFF

cmake --build "$BUILD_DIR" --target eyetrack_ffi -j"$(sysctl -n hw.logicalcpu)"

echo "=== Copying artifacts ==="

mkdir -p "$FRAMEWORKS_DIR"
cp "$BUILD_DIR/libeyetrack_ffi.dylib" "$FRAMEWORKS_DIR/"
echo "  dylib -> $FRAMEWORKS_DIR/libeyetrack_ffi.dylib"

mkdir -p "$MODEL_DIR"
if [ ! -f "$MODEL_DIR/shape_predictor_68_face_landmarks.dat" ]; then
    cp "$EYETRACK_ROOT/models/shape_predictor_68_face_landmarks.dat" "$MODEL_DIR/"
    echo "  model -> $MODEL_DIR/shape_predictor_68_face_landmarks.dat"
else
    echo "  model already exists, skipping copy"
fi

echo "=== Done ==="
echo "dylib size: $(du -h "$FRAMEWORKS_DIR/libeyetrack_ffi.dylib" | cut -f1)"
