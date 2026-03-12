# EyeTrack Models

This directory contains ONNX models used by the eyetrack pipeline.

## face_landmark.onnx

**Purpose**: Face landmark detection (468 points) for eye tracking.

**Architecture**: Based on MediaPipe FaceMesh architecture.

**Current version**: Test stub model with correct input/output shapes. Produces random
landmark positions — suitable for integration testing and pipeline validation only.
Replace with a real trained model for production use.

**Original model source**: Google MediaPipe FaceMesh
- TFLite source: `https://storage.googleapis.com/mediapipe-assets/face_landmark.tflite`
- License: Apache License 2.0
- Model card: https://developers.google.com/mediapipe/solutions/vision/face_landmarker

**Input/Output specification**:

| Tensor | Name | Shape | Type | Description |
|--------|------|-------|------|-------------|
| Input | `input_1` | `[1, 192, 192, 3]` | float32 | Cropped face region, RGB, normalized [0,1] |
| Output | `landmarks` | `[1, 1404]` | float32 | 468 landmarks x 3 (x, y, z), normalized [0,1] |
| Output | `confidence` | `[1, 1]` | float32 | Face presence confidence [0,1] |

**Preprocessing**:
1. Detect face bounding box (separate face detector needed).
2. Crop face region from frame.
3. Resize crop to 192x192.
4. Convert BGR to RGB.
5. Normalize pixel values to [0.0, 1.0] (divide by 255).

**Landmark indices for eye tracking**:
- Left eye (EAR): [362, 385, 387, 263, 373, 380]
- Right eye (EAR): [33, 160, 158, 133, 153, 144]

**SHA-256**: `c83b2b2c2d006e19c0c2018744f0e55e4df7453b10a70a9616c6a0011a7dbdb8`

**File size**: ~1.4 MB

## Replacing the stub model

To convert the real MediaPipe FaceMesh TFLite model to ONNX:

```bash
# Download the official TFLite model
curl -sL "https://storage.googleapis.com/mediapipe-assets/face_landmark.tflite" -o face_landmark.tflite

# Convert using tf2onnx (requires tensorflow + tf2onnx)
pip install tf2onnx tensorflow
python -m tf2onnx.convert --tflite face_landmark.tflite --output face_landmark.onnx --opset 13

# Or use PINTO0309's pre-converted models:
# https://github.com/PINTO0309/PINTO_model_zoo/tree/main/032_FaceMesh
```

## INT8 quantized model (for RPi)

For Raspberry Pi deployment (Phase 7), create a quantized version:

```bash
pip install onnxruntime
python -c "
from onnxruntime.quantization import quantize_dynamic, QuantType
quantize_dynamic('face_landmark.onnx', 'face_landmark_int8.onnx', weight_type=QuantType.QUInt8)
"
```
