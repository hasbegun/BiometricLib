import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';

import 'package:eyetrack/core/camera_service.dart';

void main() {
  group('CameraService', () {
    test('starts uninitialized', () {
      final service = CameraService();
      expect(service.status, CameraStatus.uninitialized);
    });

    test('default target FPS is 30', () {
      final service = CameraService();
      expect(service.targetFps, 30);
    });

    test('custom target FPS', () {
      final service = CameraService(targetFps: 15);
      expect(service.targetFps, 15);
    });

    test('frame count starts at zero', () {
      final service = CameraService();
      expect(service.frameCount, 0);
    });

    test('controller is null before initialization', () {
      final service = CameraService();
      expect(service.controller, isNull);
    });

    test('statusStream is a broadcast stream', () {
      final service = CameraService();
      service.statusStream.listen((_) {});
      service.statusStream.listen((_) {});
      // No error means broadcast works
    });

    test('frameStream is a broadcast stream', () {
      final service = CameraService();
      service.frameStream.listen((_) {});
      service.frameStream.listen((_) {});
    });

    test('stopCapture when not streaming is safe', () {
      final service = CameraService();
      service.stopCapture();
      expect(service.status, CameraStatus.uninitialized);
    });

    test('startCapture when not ready does nothing', () {
      final service = CameraService();
      service.startCapture();
      expect(service.status, CameraStatus.uninitialized);
    });
  });

  group('CameraStatus', () {
    test('has expected values', () {
      expect(CameraStatus.values.length, 5);
      expect(CameraStatus.uninitialized, isNotNull);
      expect(CameraStatus.initializing, isNotNull);
      expect(CameraStatus.ready, isNotNull);
      expect(CameraStatus.streaming, isNotNull);
      expect(CameraStatus.error, isNotNull);
    });
  });

  group('CameraFrame', () {
    test('can be constructed', () {
      final frame = CameraFrame(
        data: Uint8List.fromList([1, 2, 3]),
        frameId: 42,
        timestamp: DateTime(2025, 1, 1),
        width: 640,
        height: 480,
      );

      expect(frame.data, [1, 2, 3]);
      expect(frame.frameId, 42);
      expect(frame.width, 640);
      expect(frame.height, 480);
    });
  });
}
