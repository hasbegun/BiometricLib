import 'package:flutter_test/flutter_test.dart';

import 'package:eyetrack/core/tracking_state.dart';
import 'package:eyetrack/generated/eyetrack.pb.dart';

void main() {
  group('BlinkIndicator', () {
    test('has 4 values', () {
      expect(BlinkIndicator.values.length, 4);
    });

    test('includes none, single, double_, long', () {
      expect(BlinkIndicator.none, isNotNull);
      expect(BlinkIndicator.single, isNotNull);
      expect(BlinkIndicator.double_, isNotNull);
      expect(BlinkIndicator.long, isNotNull);
    });
  });

  group('TrackingState', () {
    test('default values', () {
      const state = TrackingState();
      expect(state.gazeX, 0.5);
      expect(state.gazeY, 0.5);
      expect(state.confidence, 0.0);
      expect(state.blink, BlinkIndicator.none);
      expect(state.fps, 0.0);
      expect(state.latencyMs, 0.0);
      expect(state.earLeft, 0.0);
      expect(state.earRight, 0.0);
      expect(state.isTracking, false);
      expect(state.debugMode, false);
    });

    test('earAverage computes mean of left and right', () {
      const state = TrackingState(earLeft: 0.3, earRight: 0.5);
      expect(state.earAverage, 0.4);
    });

    test('earAverage is zero by default', () {
      const state = TrackingState();
      expect(state.earAverage, 0.0);
    });

    test('copyWith preserves unchanged fields', () {
      const original = TrackingState(
        gazeX: 0.3,
        gazeY: 0.7,
        confidence: 0.9,
        blink: BlinkIndicator.single,
        fps: 30.0,
        latencyMs: 5.0,
        earLeft: 0.25,
        earRight: 0.28,
        isTracking: true,
        debugMode: true,
      );
      final copied = original.copyWith(gazeX: 0.4);
      expect(copied.gazeX, 0.4);
      expect(copied.gazeY, 0.7);
      expect(copied.confidence, 0.9);
      expect(copied.blink, BlinkIndicator.single);
      expect(copied.fps, 30.0);
      expect(copied.latencyMs, 5.0);
      expect(copied.earLeft, 0.25);
      expect(copied.earRight, 0.28);
      expect(copied.isTracking, true);
      expect(copied.debugMode, true);
    });

    test('copyWith updates multiple fields', () {
      const state = TrackingState();
      final updated = state.copyWith(
        gazeX: 0.1,
        gazeY: 0.9,
        confidence: 0.8,
        isTracking: true,
      );
      expect(updated.gazeX, 0.1);
      expect(updated.gazeY, 0.9);
      expect(updated.confidence, 0.8);
      expect(updated.isTracking, true);
      expect(updated.debugMode, false); // unchanged
    });

    test('copyWith changes blink indicator', () {
      const state = TrackingState();
      final updated = state.copyWith(blink: BlinkIndicator.long);
      expect(updated.blink, BlinkIndicator.long);
    });

    test('copyWith toggles debugMode', () {
      const state = TrackingState(debugMode: false);
      final toggled = state.copyWith(debugMode: true);
      expect(toggled.debugMode, true);
    });
  });

  group('blinkFromProto', () {
    test('maps BLINK_NONE to none', () {
      expect(
        TrackingState.blinkFromProto(BlinkType.BLINK_NONE),
        BlinkIndicator.none,
      );
    });

    test('maps BLINK_SINGLE to single', () {
      expect(
        TrackingState.blinkFromProto(BlinkType.BLINK_SINGLE),
        BlinkIndicator.single,
      );
    });

    test('maps BLINK_DOUBLE to double_', () {
      expect(
        TrackingState.blinkFromProto(BlinkType.BLINK_DOUBLE),
        BlinkIndicator.double_,
      );
    });

    test('maps BLINK_LONG to long', () {
      expect(
        TrackingState.blinkFromProto(BlinkType.BLINK_LONG),
        BlinkIndicator.long,
      );
    });
  });
}
