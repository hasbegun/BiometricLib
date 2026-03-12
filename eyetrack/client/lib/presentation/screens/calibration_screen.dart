import 'dart:async';
import 'dart:io' show Platform;

import 'package:camera_macos/camera_macos.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';

import 'package:eyetrack/core/calibration_state.dart';
import 'package:eyetrack/core/gaze_calibration.dart';
import 'package:eyetrack/core/native_eye_tracker.dart';
import 'package:eyetrack/l10n/app_localizations.dart';

bool get _isMacOS => !kIsWeb && Platform.isMacOS;

class CalibrationScreen extends StatefulWidget {
  const CalibrationScreen({
    super.key,
    this.onCalibrationComplete,
    this.onCancel,
    this.holdDuration = const Duration(seconds: 2),
    this.pointSize = 24.0,
    this.macController,
  });

  final void Function(List<({double x, double y})> collectedPoints, GazeCalibrationData? calibrationData)? onCalibrationComplete;
  final VoidCallback? onCancel;
  final Duration holdDuration;
  final double pointSize;
  final CameraMacOSController? macController;

  @override
  State<CalibrationScreen> createState() => CalibrationScreenState();
}

@visibleForTesting
class CalibrationScreenState extends State<CalibrationScreen>
    with SingleTickerProviderStateMixin {
  CalibrationState _state = const CalibrationState(
    phase: CalibrationPhase.instructions,
  );
  late AnimationController _holdController;
  Timer? _advanceTimer;
  final List<({double x, double y})> _collectedPoints = [];

  // Iris sampling during calibration
  final List<CalibrationSample> _irisSamples = [];
  Timer? _sampleTimer;
  bool _sampling = false;
  GazeCalibrationData? _calibrationResult;

  // Camera for iris detection during calibration
  CameraMacOSController? _macController;

  CalibrationState get state => _state;
  GazeCalibrationData? get calibrationData => _calibrationResult;

  @override
  void initState() {
    super.initState();
    _macController = widget.macController;
    _holdController = AnimationController(
      vsync: this,
      duration: widget.holdDuration,
    );
    _holdController.addListener(() {
      setState(() {
        _state = _state.copyWith(holdProgress: _holdController.value);
      });
    });
    _holdController.addStatusListener((status) {
      if (status == AnimationStatus.completed) {
        _onPointCollected();
      }
    });
  }

  @override
  void dispose() {
    _holdController.dispose();
    _advanceTimer?.cancel();
    _sampleTimer?.cancel();
    super.dispose();
  }

  void startCalibration() {
    setState(() {
      _state = const CalibrationState(
        phase: CalibrationPhase.collecting,
        currentPointIndex: 0,
      );
      _collectedPoints.clear();
      _irisSamples.clear();
    });
    _startSamplingIris();
    _holdController.forward(from: 0);
  }

  void _startSamplingIris() {
    _sampling = true;
    // Sample iris every 200ms during hold
    _sampleTimer?.cancel();
    _sampleTimer = Timer.periodic(const Duration(milliseconds: 200), (_) {
      if (_sampling && _isMacOS) {
        _captureIrisSample();
      }
    });
  }

  void _stopSamplingIris() {
    _sampling = false;
    _sampleTimer?.cancel();
  }

  Future<void> _captureIrisSample() async {
    if (_macController == null || _macController!.isDestroyed) return;

    try {
      // Take a picture for iris detection
      final file = await _macController!.takePicture();
      if (file == null || file.bytes == null) return;

      final result = await NativeEyeTracker.detectGaze(file.bytes!);

      if (result.detected && mounted && _sampling) {
        final point = CalibrationState.defaultPoints[_state.currentPointIndex];
        _irisSamples.add(CalibrationSample(
          screenX: point.x,
          screenY: point.y,
          irisH: result.irisRatioH,
          irisV: result.irisRatioV,
        ));
      }
    } catch (_) {
      // Skip failed samples
    }
  }

  void _onPointCollected() {
    final point = CalibrationState.defaultPoints[_state.currentPointIndex];
    _collectedPoints.add(point);

    if (_state.currentPointIndex + 1 >= _state.totalPoints) {
      _stopSamplingIris();

      // Build calibration data from collected iris samples
      _calibrationResult = GazeCalibrationData.fromSamples(_irisSamples);

      setState(() {
        _state = _state.copyWith(phase: CalibrationPhase.processing);
      });

      // Check if we got enough data
      if (_irisSamples.length >= 4 && _calibrationResult!.isValid) {
        Future.delayed(const Duration(milliseconds: 500), () {
          if (mounted) {
            setComplete(
              _calibrationResult!.rangeH * 100,
            );
            widget.onCalibrationComplete?.call(_collectedPoints, _calibrationResult);
          }
        });
      } else if (_irisSamples.isEmpty) {
        // No camera / no samples — still complete with default mapping
        Future.delayed(const Duration(milliseconds: 500), () {
          if (mounted) {
            _calibrationResult = const GazeCalibrationData(
              minIrisH: 0.35, maxIrisH: 0.65,
              minIrisV: 0.35, maxIrisV: 0.65,
            );
            setComplete(0);
            widget.onCalibrationComplete?.call(_collectedPoints, _calibrationResult);
          }
        });
      } else {
        Future.delayed(const Duration(milliseconds: 500), () {
          if (mounted) {
            setFailed('Not enough iris data collected. Try again with better lighting.');
          }
        });
      }
    } else {
      setState(() {
        _state = _state.copyWith(
          currentPointIndex: _state.currentPointIndex + 1,
          holdProgress: 0.0,
        );
      });
      _holdController.forward(from: 0);
    }
  }

  void setComplete(double accuracy) {
    setState(() {
      _state = _state.copyWith(
        phase: CalibrationPhase.complete,
        accuracy: accuracy,
      );
    });
  }

  void setFailed(String message) {
    setState(() {
      _state = _state.copyWith(
        phase: CalibrationPhase.failed,
        errorMessage: message,
      );
    });
  }

  void cancel() {
    _holdController.stop();
    _advanceTimer?.cancel();
    _stopSamplingIris();
    setState(() {
      _state = const CalibrationState(phase: CalibrationPhase.idle);
    });
    widget.onCancel?.call();
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);

    return Scaffold(
      appBar: AppBar(
        title: Text(l10n.calibrationTitle),
        leading: IconButton(
          icon: const Icon(Icons.close),
          onPressed: cancel,
        ),
      ),
      body: _buildBody(context, l10n),
    );
  }

  Widget _buildBody(BuildContext context, AppLocalizations l10n) {
    switch (_state.phase) {
      case CalibrationPhase.idle:
        return const SizedBox.shrink();

      case CalibrationPhase.instructions:
        return _InstructionsView(
          instructions: l10n.calibrationInstructions,
          onStart: startCalibration,
          startLabel: l10n.calibrate,
        );

      case CalibrationPhase.collecting:
        return _CalibrationPointView(
          point: CalibrationState.defaultPoints[_state.currentPointIndex],
          progress: _state.holdProgress,
          pointIndex: _state.currentPointIndex,
          totalPoints: _state.totalPoints,
          pointSize: widget.pointSize,
          sampleCount: _irisSamples.length,
        );

      case CalibrationPhase.processing:
        return Center(
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              const CircularProgressIndicator(),
              const SizedBox(height: 16),
              Text(l10n.connecting),
            ],
          ),
        );

      case CalibrationPhase.complete:
        return _ResultView(
          message: l10n.calibrationComplete,
          accuracy: _state.accuracy,
          accuracyLabel: _state.accuracy != null
              ? l10n.accuracy(_state.accuracy!.toStringAsFixed(1))
              : null,
          isSuccess: true,
          calibrationInfo: _calibrationResult?.toString(),
        );

      case CalibrationPhase.failed:
        return _ResultView(
          message: _state.errorMessage ?? l10n.calibrationFailed,
          isSuccess: false,
          retryLabel: l10n.retry,
          onRetry: () {
            setState(() {
              _state = const CalibrationState(
                phase: CalibrationPhase.instructions,
              );
            });
          },
        );
    }
  }
}

class _InstructionsView extends StatelessWidget {
  const _InstructionsView({
    required this.instructions,
    required this.onStart,
    required this.startLabel,
  });

  final String instructions;
  final VoidCallback onStart;
  final String startLabel;

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(32),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Icon(Icons.visibility, size: 64),
            const SizedBox(height: 24),
            Text(
              instructions,
              textAlign: TextAlign.center,
              style: Theme.of(context).textTheme.bodyLarge,
            ),
            const SizedBox(height: 32),
            FilledButton.icon(
              onPressed: onStart,
              icon: const Icon(Icons.play_arrow),
              label: Text(startLabel),
            ),
          ],
        ),
      ),
    );
  }
}

class _CalibrationPointView extends StatelessWidget {
  const _CalibrationPointView({
    required this.point,
    required this.progress,
    required this.pointIndex,
    required this.totalPoints,
    required this.pointSize,
    this.sampleCount = 0,
  });

  final ({double x, double y}) point;
  final double progress;
  final int pointIndex;
  final int totalPoints;
  final double pointSize;
  final int sampleCount;

  @override
  Widget build(BuildContext context) {
    return Stack(
      children: [
        // Progress indicator at top
        Positioned(
          top: 0,
          left: 0,
          right: 0,
          child: LinearProgressIndicator(
            value: (pointIndex + progress) / totalPoints,
          ),
        ),
        // Point counter + sample count
        Positioned(
          top: 16,
          right: 16,
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.end,
            children: [
              Text(
                '${pointIndex + 1} / $totalPoints',
                style: Theme.of(context).textTheme.bodySmall,
              ),
              Text(
                '$sampleCount samples',
                style: Theme.of(context).textTheme.bodySmall?.copyWith(
                  color: Theme.of(context).colorScheme.outline,
                ),
              ),
            ],
          ),
        ),
        // Calibration point
        Positioned(
          left: point.x * MediaQuery.of(context).size.width - pointSize / 2,
          top: point.y * MediaQuery.of(context).size.height - pointSize / 2,
          child: SizedBox(
            width: pointSize,
            height: pointSize,
            child: Stack(
              alignment: Alignment.center,
              children: [
                CircularProgressIndicator(
                  value: progress,
                  strokeWidth: 3,
                ),
                Container(
                  width: pointSize * 0.4,
                  height: pointSize * 0.4,
                  decoration: BoxDecoration(
                    shape: BoxShape.circle,
                    color: Theme.of(context).colorScheme.primary,
                  ),
                ),
              ],
            ),
          ),
        ),
      ],
    );
  }
}

class _ResultView extends StatelessWidget {
  const _ResultView({
    required this.message,
    required this.isSuccess,
    this.accuracy,
    this.accuracyLabel,
    this.retryLabel,
    this.onRetry,
    this.calibrationInfo,
  });

  final String message;
  final bool isSuccess;
  final double? accuracy;
  final String? accuracyLabel;
  final String? retryLabel;
  final VoidCallback? onRetry;
  final String? calibrationInfo;

  @override
  Widget build(BuildContext context) {
    final icon = isSuccess ? Icons.check_circle : Icons.error;
    final color = isSuccess
        ? Theme.of(context).colorScheme.primary
        : Theme.of(context).colorScheme.error;

    return Center(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(icon, size: 64, color: color),
          const SizedBox(height: 16),
          Text(
            message,
            textAlign: TextAlign.center,
            style: Theme.of(context).textTheme.headlineSmall,
          ),
          if (accuracyLabel != null) ...[
            const SizedBox(height: 8),
            Text(
              accuracyLabel!,
              style: Theme.of(context).textTheme.bodyLarge,
            ),
          ],
          if (calibrationInfo != null) ...[
            const SizedBox(height: 8),
            Text(
              calibrationInfo!,
              style: Theme.of(context).textTheme.bodySmall?.copyWith(
                color: Theme.of(context).colorScheme.outline,
              ),
            ),
          ],
          if (onRetry != null) ...[
            const SizedBox(height: 24),
            FilledButton.icon(
              onPressed: onRetry,
              icon: const Icon(Icons.refresh),
              label: Text(retryLabel ?? 'Retry'),
            ),
          ],
        ],
      ),
    );
  }
}
