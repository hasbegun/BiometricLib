import 'package:fixnum/fixnum.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:eyetrack/generated/eyetrack.pbgrpc.dart';

void main() {
  group('Protobuf message creation', () {
    test('FrameData can be constructed', () {
      final frame = FrameData()
        ..frameId = Int64(1)
        ..timestampNs = Int64(DateTime.now().microsecondsSinceEpoch * 1000)
        ..clientId = 'test_client'
        ..width = 640
        ..height = 480
        ..channels = 3;

      expect(frame.frameId, Int64(1));
      expect(frame.clientId, 'test_client');
      expect(frame.width, 640);
      expect(frame.height, 480);
      expect(frame.channels, 3);
    });

    test('GazeEvent can be constructed', () {
      final gaze = GazeEvent()
        ..gazePoint = (Point2D()
          ..x = 0.5
          ..y = 0.5)
        ..confidence = 0.95
        ..blink = BlinkType.BLINK_NONE
        ..clientId = 'test_client'
        ..frameId = Int64(42);

      expect(gaze.gazePoint.x, 0.5);
      expect(gaze.gazePoint.y, 0.5);
      expect(gaze.confidence, closeTo(0.95, 0.001));
      expect(gaze.blink, BlinkType.BLINK_NONE);
    });

    test('BlinkType enum has expected values', () {
      expect(BlinkType.BLINK_NONE.value, 0);
      expect(BlinkType.BLINK_SINGLE.value, 1);
      expect(BlinkType.BLINK_DOUBLE.value, 2);
      expect(BlinkType.BLINK_LONG.value, 3);
    });

    test('CalibrationRequest can hold calibration points', () {
      final request = CalibrationRequest()
        ..userId = 'user_1'
        ..points.addAll([
          CalibrationPoint()
            ..screenPoint = (Point2D()
              ..x = 0.1
              ..y = 0.1)
            ..pupilObservation = (PupilInfo()
              ..center = (Point2D()
                ..x = 320.0
                ..y = 240.0)
              ..radius = 15.0
              ..confidence = 0.9),
          CalibrationPoint()
            ..screenPoint = (Point2D()
              ..x = 0.9
              ..y = 0.9)
            ..pupilObservation = (PupilInfo()
              ..center = (Point2D()
                ..x = 350.0
                ..y = 260.0)
              ..radius = 14.5
              ..confidence = 0.88),
        ]);

      expect(request.userId, 'user_1');
      expect(request.points.length, 2);
      expect(request.points[0].screenPoint.x, closeTo(0.1, 0.001));
      expect(request.points[1].pupilObservation.radius, closeTo(14.5, 0.01));
    });

    test('FeatureData contains eye landmarks and pupil info', () {
      final features = FeatureData()
        ..earLeft = 0.3
        ..earRight = 0.28
        ..frameId = Int64(100)
        ..leftPupil = (PupilInfo()
          ..center = (Point2D()
            ..x = 200.0
            ..y = 150.0)
          ..radius = 12.0
          ..confidence = 0.95)
        ..rightPupil = (PupilInfo()
          ..center = (Point2D()
            ..x = 400.0
            ..y = 155.0)
          ..radius = 11.5
          ..confidence = 0.92);

      expect(features.earLeft, closeTo(0.3, 0.001));
      expect(features.earRight, closeTo(0.28, 0.001));
      expect(features.leftPupil.radius, closeTo(12.0, 0.01));
      expect(features.rightPupil.confidence, closeTo(0.92, 0.001));
    });

    test('Protobuf serialization roundtrip', () {
      final original = GazeEvent()
        ..gazePoint = (Point2D()
          ..x = 0.75
          ..y = 0.25)
        ..confidence = 0.99
        ..blink = BlinkType.BLINK_DOUBLE
        ..timestampNs = Int64(1234567890)
        ..clientId = 'roundtrip_test'
        ..frameId = Int64(99);

      final bytes = original.writeToBuffer();
      final decoded = GazeEvent.fromBuffer(bytes);

      expect(decoded.gazePoint.x, closeTo(original.gazePoint.x, 0.001));
      expect(decoded.gazePoint.y, closeTo(original.gazePoint.y, 0.001));
      expect(decoded.confidence, closeTo(original.confidence, 0.001));
      expect(decoded.blink, original.blink);
      expect(decoded.timestampNs, original.timestampNs);
      expect(decoded.clientId, original.clientId);
      expect(decoded.frameId, original.frameId);
    });

    test('StatusResponse can be constructed', () {
      final status = StatusResponse()
        ..isRunning = true
        ..connectedClients = 5
        ..fps = 30.0
        ..version = '1.0.0';

      expect(status.isRunning, isTrue);
      expect(status.connectedClients, 5);
      expect(status.fps, closeTo(30.0, 0.01));
      expect(status.version, '1.0.0');
    });

    test('gRPC service client class exists', () {
      expect(EyeTrackServiceClient, isNotNull);
    });
  });
}
