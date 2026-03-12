import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

import 'package:eyetrack/core/eyetrack_ffi.dart';

/// Result from native Vision framework eye detection.
@immutable
class EyeTrackResult {
  const EyeTrackResult({
    required this.detected,
    this.gazeX = 0.5,
    this.gazeY = 0.5,
    this.confidence = 0.0,
    this.vecX = 0.0,
    this.vecY = 0.0,
    this.earLeft = 0.3,
    this.earRight = 0.3,
    this.leftEyeCenter = const [0.5, 0.5],
    this.rightEyeCenter = const [0.5, 0.5],
    this.leftPupilPos = const [0.5, 0.5],
    this.rightPupilPos = const [0.5, 0.5],
    this.faceBox = const [0, 0, 1, 1],
    this.yaw = 0.0,
    this.pupilOffX = 0.0,
    this.pupilOffY = 0.0,
    this.irisRatioH = 0.5,
    this.irisRatioV = 0.5,
  });

  final bool detected;
  final double gazeX;
  final double gazeY;
  final double confidence;
  final double vecX;
  final double vecY;
  final double earLeft;
  final double earRight;
  final List<double> leftEyeCenter;
  final List<double> rightEyeCenter;
  final List<double> leftPupilPos;
  final List<double> rightPupilPos;
  final List<double> faceBox;
  final double yaw;
  final double pupilOffX;
  final double pupilOffY;
  final double irisRatioH;
  final double irisRatioV;

  factory EyeTrackResult.fromMap(Map<dynamic, dynamic> map) {
    final detected = map['detected'] as bool? ?? false;
    if (!detected) {
      return const EyeTrackResult(detected: false);
    }

    return EyeTrackResult(
      detected: true,
      gazeX: (map['gazeX'] as num?)?.toDouble() ?? 0.5,
      gazeY: (map['gazeY'] as num?)?.toDouble() ?? 0.5,
      confidence: (map['confidence'] as num?)?.toDouble() ?? 0.0,
      vecX: (map['vecX'] as num?)?.toDouble() ?? 0.0,
      vecY: (map['vecY'] as num?)?.toDouble() ?? 0.0,
      earLeft: (map['earLeft'] as num?)?.toDouble() ?? 0.3,
      earRight: (map['earRight'] as num?)?.toDouble() ?? 0.3,
      leftEyeCenter: _toDoubleList(map['leftEyeCenter']),
      rightEyeCenter: _toDoubleList(map['rightEyeCenter']),
      leftPupilPos: _toDoubleList(map['leftPupilPos']),
      rightPupilPos: _toDoubleList(map['rightPupilPos']),
      faceBox: _toDoubleList(map['faceBox'], fallback: [0, 0, 1, 1]),
      yaw: (map['yaw'] as num?)?.toDouble() ?? 0.0,
      pupilOffX: (map['pupilOffX'] as num?)?.toDouble() ?? 0.0,
      pupilOffY: (map['pupilOffY'] as num?)?.toDouble() ?? 0.0,
      irisRatioH: (map['irisRatioH'] as num?)?.toDouble() ?? 0.5,
      irisRatioV: (map['irisRatioV'] as num?)?.toDouble() ?? 0.5,
    );
  }

  static List<double> _toDoubleList(dynamic value, {List<double> fallback = const [0.5, 0.5]}) {
    if (value is List) {
      return value.map((e) => (e as num).toDouble()).toList();
    }
    return fallback;
  }
}

/// Communicates with native macOS Vision framework for eye tracking.
class NativeEyeTracker {
  static const _channel = MethodChannel('eyetrack/native_eye_tracker');

  /// Detect gaze from raw ARGB8888 frame data (from startImageStream).
  static Future<EyeTrackResult> detectGazeRaw({
    required Uint8List imageData,
    required int width,
    required int height,
    required int bytesPerRow,
  }) async {
    try {
      final result = await _channel.invokeMethod<Map<dynamic, dynamic>>(
        'detectGazeRaw',
        {
          'imageData': imageData,
          'width': width,
          'height': height,
          'bytesPerRow': bytesPerRow,
        },
      );
      if (result == null) {
        return const EyeTrackResult(detected: false);
      }
      return EyeTrackResult.fromMap(result);
    } on PlatformException {
      return const EyeTrackResult(detected: false);
    } on MissingPluginException {
      return const EyeTrackResult(detected: false);
    }
  }

  /// Detect gaze from encoded image data (JPEG/PNG/TIFF from takePicture).
  static Future<EyeTrackResult> detectGaze(Uint8List imageData) async {
    try {
      final result = await _channel.invokeMethod<Map<dynamic, dynamic>>(
        'detectGaze',
        {'imageData': imageData},
      );
      if (result == null) {
        return const EyeTrackResult(detected: false);
      }
      return EyeTrackResult.fromMap(result);
    } on PlatformException {
      return const EyeTrackResult(detected: false);
    } on MissingPluginException {
      return const EyeTrackResult(detected: false);
    }
  }
}

/// Eye tracker using C++ OpenCV+dlib via dart:ffi.
/// Falls back to MethodChannel if FFI initialization fails.
class NativeEyeTrackerFFI {
  static final EyetrackFFI _ffi = EyetrackFFI();
  static bool _initialized = false;
  static bool _initFailed = false;

  static bool get isAvailable => _initialized && !_initFailed;

  /// Initialize the FFI eye tracker. Safe to call multiple times.
  static void initialize() {
    if (_initialized || _initFailed) return;

    try {
      final modelDir = EyetrackFFI.findModelDir();
      if (modelDir == null) {
        debugPrint('EyetrackFFI: model directory not found, falling back to MethodChannel');
        _initFailed = true;
        return;
      }

      _ffi.initialize(modelDir);
      _initialized = true;
      debugPrint('EyetrackFFI: initialized successfully with models from $modelDir');
    } catch (e) {
      debugPrint('EyetrackFFI: initialization failed: $e');
      _initFailed = true;
    }
  }

  /// Detect gaze from raw ARGB8888 frame data.
  /// Synchronous — no MethodChannel overhead.
  static EyeTrackResult detectGazeRaw({
    required Uint8List imageData,
    required int width,
    required int height,
    required int bytesPerRow,
  }) {
    if (!_initialized) {
      return const EyeTrackResult(detected: false);
    }

    try {
      final result = _ffi.processFrame(imageData, width, height, bytesPerRow);

      if (result.detected == 0) {
        return const EyeTrackResult(detected: false);
      }

      return EyeTrackResult(
        detected: true,
        gazeX: result.gazeX,
        gazeY: result.gazeY,
        confidence: result.confidence,
        earLeft: result.earLeft,
        earRight: result.earRight,
        irisRatioH: result.irisRatioH,
        irisRatioV: result.irisRatioV,
        leftPupilPos: [result.leftPupilX, result.leftPupilY],
        rightPupilPos: [result.rightPupilX, result.rightPupilY],
        faceBox: [result.faceX, result.faceY, result.faceW, result.faceH],
      );
    } catch (e) {
      debugPrint('EyetrackFFI: processFrame error: $e');
      return const EyeTrackResult(detected: false);
    }
  }

  /// Dispose the FFI resources.
  static void dispose() {
    if (_initialized) {
      _ffi.dispose();
      _initialized = false;
    }
  }
}
