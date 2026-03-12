import 'dart:async';

import 'package:camera/camera.dart';
import 'package:flutter/foundation.dart';

enum CameraStatus {
  uninitialized,
  initializing,
  ready,
  streaming,
  error,
}

class CameraService {
  CameraService({this.targetFps = 30});

  final int targetFps;

  CameraController? _controller;
  CameraStatus _status = CameraStatus.uninitialized;
  final _statusController = StreamController<CameraStatus>.broadcast();
  final _frameController = StreamController<CameraFrame>.broadcast();
  Timer? _captureTimer;
  int _frameCount = 0;

  CameraStatus get status => _status;
  Stream<CameraStatus> get statusStream => _statusController.stream;
  Stream<CameraFrame> get frameStream => _frameController.stream;
  CameraController? get controller => _controller;
  int get frameCount => _frameCount;

  void _setStatus(CameraStatus status) {
    _status = status;
    _statusController.add(status);
  }

  Future<void> initialize({
    CameraLensDirection direction = CameraLensDirection.front,
    ResolutionPreset resolution = ResolutionPreset.medium,
  }) async {
    if (_status == CameraStatus.ready || _status == CameraStatus.streaming) {
      return;
    }
    _setStatus(CameraStatus.initializing);

    try {
      final cameras = await availableCameras();
      if (cameras.isEmpty) {
        _setStatus(CameraStatus.error);
        throw CameraException('noCameras', 'No cameras available on this device.');
      }

      final camera = cameras.firstWhere(
        (c) => c.lensDirection == direction,
        orElse: () => cameras.first,
      );

      _controller = CameraController(
        camera,
        resolution,
        enableAudio: false,
        imageFormatGroup: ImageFormatGroup.jpeg,
      );

      await _controller!.initialize();
      _setStatus(CameraStatus.ready);
    } catch (e) {
      _setStatus(CameraStatus.error);
      rethrow;
    }
  }

  void startCapture() {
    if (_status != CameraStatus.ready) return;
    _setStatus(CameraStatus.streaming);

    final interval = Duration(milliseconds: (1000 / targetFps).round());
    _captureTimer = Timer.periodic(interval, (_) => _captureFrame());
  }

  void stopCapture() {
    _captureTimer?.cancel();
    _captureTimer = null;
    if (_status == CameraStatus.streaming) {
      _setStatus(CameraStatus.ready);
    }
  }

  Future<void> _captureFrame() async {
    if (_controller == null || !_controller!.value.isInitialized) return;

    try {
      final xFile = await _controller!.takePicture();
      final bytes = await xFile.readAsBytes();
      _frameCount++;
      _frameController.add(CameraFrame(
        data: bytes,
        frameId: _frameCount,
        timestamp: DateTime.now(),
        width: _controller!.value.previewSize?.width.toInt() ?? 0,
        height: _controller!.value.previewSize?.height.toInt() ?? 0,
      ));
    } catch (_) {
      // Skip frame on error (camera busy, etc.)
    }
  }

  Future<void> dispose() async {
    stopCapture();
    await _controller?.dispose();
    _controller = null;
    _setStatus(CameraStatus.uninitialized);
    await _statusController.close();
    await _frameController.close();
  }
}

@immutable
class CameraFrame {
  const CameraFrame({
    required this.data,
    required this.frameId,
    required this.timestamp,
    required this.width,
    required this.height,
  });

  final Uint8List data;
  final int frameId;
  final DateTime timestamp;
  final int width;
  final int height;
}
