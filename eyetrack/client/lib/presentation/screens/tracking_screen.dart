import 'dart:async';
import 'dart:io' show Platform;
import 'dart:math' as math;

import 'package:camera_macos/camera_macos.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';

import 'package:eyetrack/core/camera_service.dart';
import 'package:eyetrack/core/gaze_calibration.dart';
import 'package:eyetrack/core/native_eye_tracker.dart';
import 'package:eyetrack/core/tracking_state.dart';
import 'package:eyetrack/l10n/app_localizations.dart';

bool get _isMacOS => !kIsWeb && Platform.isMacOS;

class TrackingScreen extends StatefulWidget {
  const TrackingScreen({
    super.key,
    this.onStartTracking,
    this.onStopTracking,
    this.cameraService,
    this.calibrationData,
  });

  final VoidCallback? onStartTracking;
  final VoidCallback? onStopTracking;
  final CameraService? cameraService;
  final GazeCalibrationData? calibrationData;

  @override
  State<TrackingScreen> createState() => TrackingScreenState();
}

class TrackingScreenState extends State<TrackingScreen> {
  TrackingState _state = const TrackingState();
  CameraService? _cameraService;
  bool _cameraOwned = false;
  bool _macCameraReady = false;
  CameraMacOSController? _macController;
  StreamSubscription<CameraStatus>? _statusSub;
  StreamSubscription<CameraFrame>? _frameSub;

  // Eye tracking
  EyeTrackResult? _lastEyeResult;
  bool _processing = false;
  int _frameCount = 0;
  DateTime? _fpsWindowStart;

  // Throttle: minimum interval between processing
  static const Duration _detectInterval = Duration(milliseconds: 200);
  DateTime _lastDetectTime = DateTime(2000);

  // Whether to use C++ FFI (preferred) or Swift MethodChannel (fallback)
  bool _useFFI = false;

  // Calibration
  GazeCalibrationData? _calibrationData;

  TrackingState get state => _state;
  CameraService? get cameraService => _cameraService;
  CameraMacOSController? get macCameraController => _macController;

  void setCalibration(GazeCalibrationData data) {
    setState(() {
      _calibrationData = data;
    });
  }

  int get gazeTile {
    final col = (_state.gazeX * 8).floor().clamp(0, 7);
    final row = (_state.gazeY * 8).floor().clamp(0, 7);
    return row * 8 + col + 1;
  }

  @override
  void initState() {
    super.initState();
    _calibrationData = widget.calibrationData;

    // Try to initialize C++ FFI eye tracker
    if (_isMacOS) {
      NativeEyeTrackerFFI.initialize();
      _useFFI = NativeEyeTrackerFFI.isAvailable;
    }

    if (!_isMacOS) {
      if (widget.cameraService != null) {
        _cameraService = widget.cameraService!;
      } else {
        _cameraService = CameraService(targetFps: 15);
        _cameraOwned = true;
      }
      _statusSub = _cameraService!.statusStream.listen(_onCameraStatus);
      _frameSub = _cameraService!.frameStream.listen(_onFrame);
      if (_cameraOwned) {
        _initCamera();
      }
    }
  }

  Future<void> _initCamera() async {
    try {
      await _cameraService!.initialize();
    } catch (_) {
      if (mounted) setState(() {});
    }
  }

  void _onCameraStatus(CameraStatus status) {
    if (!mounted) return;
    setState(() {});
  }

  void _onFrame(CameraFrame frame) {
    if (!mounted) return;
    _updateFps();
  }

  void _updateFps() {
    final now = DateTime.now();
    _fpsWindowStart ??= now;
    _frameCount++;
    final elapsed = now.difference(_fpsWindowStart!).inMilliseconds;
    if (elapsed >= 1000) {
      final fps = _frameCount * 1000.0 / elapsed;
      _fpsWindowStart = now;
      _frameCount = 0;
      if (mounted) {
        setState(() {
          _state = _state.copyWith(fps: fps);
        });
      }
    }
  }

  void _onMacCameraReady(CameraMacOSController controller) {
    _macController = controller;
    if (mounted) {
      setState(() => _macCameraReady = true);
    }
  }

  void _startImageStream() {
    if (_macController == null || _macController!.isDestroyed) return;
    _macController!.startImageStream(
      (CameraImageData? imageData) {
        if (imageData != null) _onImageFrame(imageData);
      },
    );
  }

  void _stopImageStream() {
    if (_macController == null || _macController!.isDestroyed) return;
    if (_macController!.isStreamingImageData) {
      _macController!.stopImageStream();
    }
  }

  void _onImageFrame(CameraImageData imageData) {
    if (_processing || !mounted || !_state.isTracking) return;

    // Throttle: skip frames until interval has passed
    final now = DateTime.now();
    if (now.difference(_lastDetectTime) < _detectInterval) return;
    _lastDetectTime = now;
    _processing = true;

    _updateFps();
    _detectFromRawFrame(imageData);
  }

  Future<void> _detectFromRawFrame(CameraImageData imageData) async {
    try {
      final startTime = DateTime.now();

      // Use C++ FFI (synchronous) or Swift MethodChannel (async)
      final EyeTrackResult result;
      if (_useFFI) {
        result = NativeEyeTrackerFFI.detectGazeRaw(
          imageData: imageData.bytes,
          width: imageData.width,
          height: imageData.height,
          bytesPerRow: imageData.bytesPerRow,
        );
      } else {
        result = await NativeEyeTracker.detectGazeRaw(
          imageData: imageData.bytes,
          width: imageData.width,
          height: imageData.height,
          bytesPerRow: imageData.bytesPerRow,
        );
      }

      final latency = DateTime.now().difference(startTime).inMilliseconds.toDouble();

      if (!mounted || !_state.isTracking) return;

      setState(() {
        _lastEyeResult = result;
        if (result.detected) {
          final earAvg = (result.earLeft + result.earRight) / 2.0;

          // Blink detection — freeze gaze when eyes are closed
          if (earAvg < 0.15) {
            if (_state.blink == BlinkIndicator.none) {
              updateBlink(BlinkIndicator.single);
            }
            _state = _state.copyWith(
              earLeft: result.earLeft,
              earRight: result.earRight,
              latencyMs: latency,
            );
            return;
          }

          // FFI path: C++ already smooths gaze, use directly
          // MethodChannel path: apply calibration if available
          double gazeX;
          double gazeY;
          if (_useFFI) {
            gazeX = result.gazeX;
            gazeY = result.gazeY;
          } else if (_calibrationData != null) {
            gazeX = _calibrationData!.mapH(result.irisRatioH);
            gazeY = _calibrationData!.mapV(result.irisRatioV);
          } else {
            gazeX = result.gazeX;
            gazeY = result.gazeY;
          }

          _state = _state.copyWith(
            gazeX: gazeX,
            gazeY: gazeY,
            confidence: result.confidence,
            earLeft: result.earLeft,
            earRight: result.earRight,
            latencyMs: latency,
          );
        }
      });
    } catch (_) {
      // Skip frame on error
    } finally {
      _processing = false;
    }
  }

  @override
  void dispose() {
    _stopImageStream();
    _statusSub?.cancel();
    _frameSub?.cancel();
    if (_cameraOwned && _cameraService != null) {
      _cameraService!.dispose();
    }
    _macController?.destroy();
    NativeEyeTrackerFFI.dispose();
    super.dispose();
  }

  void updateGaze(double x, double y, double confidence) {
    setState(() {
      _state = _state.copyWith(gazeX: x, gazeY: y, confidence: confidence);
    });
  }

  void updateBlink(BlinkIndicator blink) {
    setState(() {
      _state = _state.copyWith(blink: blink);
    });
    if (blink != BlinkIndicator.none) {
      Future.delayed(const Duration(milliseconds: 500), () {
        if (mounted) {
          setState(() {
            _state = _state.copyWith(blink: BlinkIndicator.none);
          });
        }
      });
    }
  }

  void updateDebugInfo({double? fps, double? latencyMs, double? earLeft, double? earRight}) {
    setState(() {
      _state = _state.copyWith(
        fps: fps, latencyMs: latencyMs,
        earLeft: earLeft, earRight: earRight,
      );
    });
  }

  void toggleDebugMode() {
    setState(() {
      _state = _state.copyWith(debugMode: !_state.debugMode);
    });
  }

  void setTracking(bool tracking) {
    setState(() {
      _state = _state.copyWith(isTracking: tracking);
    });
    if (_isMacOS) {
      if (tracking) {
        _startImageStream();
      } else {
        _stopImageStream();
      }
    } else if (_cameraService != null) {
      if (tracking) {
        _cameraService!.startCapture();
      } else {
        _cameraService!.stopCapture();
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);

    return Scaffold(
      appBar: AppBar(
        title: Text(l10n.homeTitle),
        actions: [
          IconButton(
            icon: Icon(_state.debugMode ? Icons.bug_report : Icons.bug_report_outlined),
            onPressed: toggleDebugMode,
            tooltip: l10n.debugMode,
          ),
        ],
      ),
      body: Column(
        children: [
          // 8x8 grid — takes all available space
          Expanded(
            child: _GazeGrid(
              highlightTile: _state.isTracking ? gazeTile : null,
              confidence: _state.confidence,
            ),
          ),

          // Bottom bar: camera preview + tile display + controls
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
            decoration: BoxDecoration(
              color: Theme.of(context).colorScheme.surfaceContainerLow,
              border: Border(
                top: BorderSide(color: Theme.of(context).colorScheme.outlineVariant),
              ),
            ),
            child: Row(
              children: [
                // Camera preview (small, left side)
                if (_isMacOS)
                  _CameraPreview(
                    eyeResult: _lastEyeResult,
                    isTracking: _state.isTracking,
                    onCameraReady: _onMacCameraReady,
                  ),

                if (_isMacOS) const SizedBox(width: 12),

                // Center: tile display + blink + debug info
                Expanded(
                  child: Column(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      // Blink indicator
                      if (_state.blink != BlinkIndicator.none)
                        Padding(
                          padding: const EdgeInsets.only(bottom: 4),
                          child: _BlinkBadge(blink: _state.blink, l10n: l10n),
                        ),

                      // Tile number
                      _TileDisplay(
                        isTracking: _state.isTracking,
                        tile: gazeTile,
                        l10n: l10n,
                      ),

                      // Debug overlay (inline)
                      if (_state.debugMode)
                        _DebugOverlay(
                          state: _state,
                          l10n: l10n,
                          cameraReady: _isMacOS
                              ? _macCameraReady
                              : (_cameraService?.status == CameraStatus.ready ||
                                  _cameraService?.status == CameraStatus.streaming),
                          gazeTile: gazeTile,
                          yaw: _lastEyeResult?.yaw ?? 0.0,
                          pupilOffX: _lastEyeResult?.irisRatioH ?? 0.5,
                          pupilOffY: _lastEyeResult?.irisRatioV ?? 0.5,
                        ),
                    ],
                  ),
                ),

                // Start/Stop button (right side)
                FilledButton.icon(
                  onPressed: () {
                    if (_state.isTracking) {
                      setTracking(false);
                      widget.onStopTracking?.call();
                    } else {
                      setTracking(true);
                      widget.onStartTracking?.call();
                    }
                  },
                  icon: Icon(_state.isTracking ? Icons.stop : Icons.play_arrow),
                  label: Text(_state.isTracking ? l10n.stopTracking : l10n.startTracking),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

/// Small camera preview with eye tracking overlay.
class _CameraPreview extends StatelessWidget {
  const _CameraPreview({
    required this.eyeResult,
    required this.isTracking,
    required this.onCameraReady,
  });

  final EyeTrackResult? eyeResult;
  final bool isTracking;
  final void Function(CameraMacOSController) onCameraReady;

  @override
  Widget build(BuildContext context) {
    return Container(
      width: 160,
      height: 120,
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(8),
        border: Border.all(
          color: isTracking
              ? (eyeResult?.detected == true ? Colors.green : Colors.orange)
              : Theme.of(context).colorScheme.outline,
          width: 2,
        ),
      ),
      child: ClipRRect(
        borderRadius: BorderRadius.circular(6),
        child: Stack(
          fit: StackFit.expand,
          children: [
            CameraMacOSView(
              key: const ValueKey('macos_camera_preview'),
              cameraMode: CameraMacOSMode.photo,
              enableAudio: false,
              fit: BoxFit.cover,
              resolution: PictureResolution.medium,
              onCameraInizialized: onCameraReady,
              onCameraLoading: (error) {
                return Container(
                  color: Colors.black,
                  child: Center(
                    child: error != null
                        ? const Icon(Icons.error, color: Colors.red, size: 24)
                        : const SizedBox(
                            width: 20, height: 20,
                            child: CircularProgressIndicator(strokeWidth: 2),
                          ),
                  ),
                );
              },
            ),

            // Eye tracking overlay
            if (isTracking && eyeResult != null && eyeResult!.detected)
              CustomPaint(
                painter: _EyeOverlayPainter(eyeResult: eyeResult!),
              ),
          ],
        ),
      ),
    );
  }
}

/// Paints eye landmarks and gaze direction vectors on the camera preview.
class _EyeOverlayPainter extends CustomPainter {
  _EyeOverlayPainter({required this.eyeResult});

  final EyeTrackResult eyeResult;

  @override
  void paint(Canvas canvas, Size size) {
    // Vision: (0,0)=bottom-left, (1,1)=top-right; camera mirrored
    Offset toScreen(List<double> pt) {
      return Offset((1.0 - pt[0]) * size.width, (1.0 - pt[1]) * size.height);
    }

    final leftEye = toScreen(eyeResult.leftEyeCenter);
    final rightEye = toScreen(eyeResult.rightEyeCenter);

    // Face box
    final fb = eyeResult.faceBox;
    final faceRect = Rect.fromLTWH(
      (1.0 - fb[0] - fb[2]) * size.width,
      (1.0 - fb[1] - fb[3]) * size.height,
      fb[2] * size.width, fb[3] * size.height,
    );
    canvas.drawRect(faceRect, Paint()
      ..color = Colors.green.withAlpha(100)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1.0);

    // Eye circles
    final eyePaint = Paint()
      ..color = Colors.cyan
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1.5;
    final r = size.width * 0.05;
    canvas.drawCircle(leftEye, r, eyePaint);
    canvas.drawCircle(rightEye, r, eyePaint);

    // Pupil dots
    final leftPupil = toScreen(eyeResult.leftPupilPos);
    final rightPupil = toScreen(eyeResult.rightPupilPos);
    final pupilPaint = Paint()..color = Colors.yellow..style = PaintingStyle.fill;
    canvas.drawCircle(leftPupil, 2.5, pupilPaint);
    canvas.drawCircle(rightPupil, 2.5, pupilPaint);

    // Gaze direction arrows
    final arrowLen = size.width * 0.12;
    final arrowPaint = Paint()
      ..color = Colors.red
      ..style = PaintingStyle.stroke
      ..strokeWidth = 2.0
      ..strokeCap = StrokeCap.round;

    for (final eye in [leftEye, rightEye]) {
      final end = Offset(
        eye.dx + eyeResult.vecX * arrowLen,
        eye.dy - eyeResult.vecY * arrowLen,
      );
      canvas.drawLine(eye, end, arrowPaint);
      final angle = math.atan2(end.dy - eye.dy, end.dx - eye.dx);
      final hl = arrowLen * 0.3;
      canvas.drawLine(end, Offset(
        end.dx - hl * math.cos(angle - 0.4),
        end.dy - hl * math.sin(angle - 0.4),
      ), arrowPaint);
      canvas.drawLine(end, Offset(
        end.dx - hl * math.cos(angle + 0.4),
        end.dy - hl * math.sin(angle + 0.4),
      ), arrowPaint);
    }
  }

  @override
  bool shouldRepaint(covariant _EyeOverlayPainter old) => true;
}

/// 8x8 chess-like grid numbered 1-64.
class _GazeGrid extends StatelessWidget {
  const _GazeGrid({this.highlightTile, this.confidence = 0.0});

  final int? highlightTile;
  final double confidence;

  @override
  Widget build(BuildContext context) {
    final cs = Theme.of(context).colorScheme;

    return Padding(
      padding: const EdgeInsets.all(8),
      child: GridView.builder(
        physics: const NeverScrollableScrollPhysics(),
        gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
          crossAxisCount: 8, childAspectRatio: 1.0,
        ),
        itemCount: 64,
        itemBuilder: (context, index) {
          final tile = index + 1;
          final row = index ~/ 8;
          final col = index % 8;
          final isDark = (row + col) % 2 == 1;
          final isHit = highlightTile == tile;

          Color bg;
          if (isHit) {
            bg = Color.lerp(cs.error, cs.primary, confidence.clamp(0.0, 1.0))!
                .withAlpha((confidence * 180 + 75).toInt().clamp(75, 255));
          } else {
            bg = isDark ? cs.surfaceContainerHighest : cs.surface;
          }

          return Container(
            decoration: BoxDecoration(
              color: bg,
              border: Border.all(
                color: isHit ? cs.primary : cs.outlineVariant.withAlpha(80),
                width: isHit ? 2.5 : 0.5,
              ),
            ),
            child: Center(
              child: Text(
                '$tile',
                style: TextStyle(
                  fontSize: 14,
                  fontWeight: isHit ? FontWeight.bold : FontWeight.normal,
                  color: isHit ? Colors.white
                      : (isDark ? cs.onSurfaceVariant : cs.onSurface.withAlpha(150)),
                ),
              ),
            ),
          );
        },
      ),
    );
  }
}

class _TileDisplay extends StatelessWidget {
  const _TileDisplay({required this.isTracking, required this.tile, required this.l10n});

  final bool isTracking;
  final int tile;
  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    final cs = Theme.of(context).colorScheme;
    return Text(
      isTracking ? l10n.gazeTile(tile) : l10n.noGaze,
      style: TextStyle(
        fontSize: 22, fontWeight: FontWeight.bold,
        color: isTracking ? cs.primary : cs.onSurfaceVariant,
      ),
      textAlign: TextAlign.center,
    );
  }
}

class _BlinkBadge extends StatelessWidget {
  const _BlinkBadge({required this.blink, required this.l10n});

  final BlinkIndicator blink;
  final AppLocalizations l10n;

  @override
  Widget build(BuildContext context) {
    final String label;
    final Color color;
    switch (blink) {
      case BlinkIndicator.single: label = l10n.blinkSingle; color = Colors.blue;
      case BlinkIndicator.double_: label = l10n.blinkDouble; color = Colors.orange;
      case BlinkIndicator.long: label = l10n.blinkLong; color = Colors.red;
      case BlinkIndicator.none: return const SizedBox.shrink();
    }
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 4),
      decoration: BoxDecoration(
        color: color.withAlpha(200),
        borderRadius: BorderRadius.circular(16),
      ),
      child: Text(label, style: const TextStyle(color: Colors.white, fontWeight: FontWeight.bold, fontSize: 12)),
    );
  }
}

class _DebugOverlay extends StatelessWidget {
  const _DebugOverlay({
    required this.state, required this.l10n,
    required this.cameraReady, required this.gazeTile,
    this.yaw = 0.0,
    this.pupilOffX = 0.0,
    this.pupilOffY = 0.0,
  });

  final TrackingState state;
  final AppLocalizations l10n;
  final bool cameraReady;
  final int gazeTile;
  final double yaw;
  final double pupilOffX;
  final double pupilOffY;

  @override
  Widget build(BuildContext context) {
    final style = TextStyle(fontSize: 11, color: Theme.of(context).colorScheme.onSurfaceVariant);
    return Padding(
      padding: const EdgeInsets.only(top: 4),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        mainAxisSize: MainAxisSize.min,
        children: [
          Wrap(
            spacing: 12,
            children: [
              Text('Cam: ${cameraReady ? "OK" : "—"}', style: style),
              Text(l10n.fps(state.fps.toStringAsFixed(1)), style: style),
              Text(l10n.latency(state.latencyMs.toStringAsFixed(1)), style: style),
              Text('EAR L: ${state.earLeft.toStringAsFixed(3)} R: ${state.earRight.toStringAsFixed(3)}', style: style),
              Text('Gaze: (${state.gazeX.toStringAsFixed(2)}, ${state.gazeY.toStringAsFixed(2)})', style: style),
              Text('Yaw: ${(yaw * 180 / 3.14159).toStringAsFixed(1)}°', style: style),
              Text('Iris: H${pupilOffX.toStringAsFixed(2)} V${pupilOffY.toStringAsFixed(2)}', style: style),
              Text('Tile: $gazeTile', style: style),
            ],
          ),
          const SizedBox(height: 4),
          SizedBox(
            width: 120,
            child: LinearProgressIndicator(value: state.confidence.clamp(0.0, 1.0)),
          ),
        ],
      ),
    );
  }
}
