import 'package:flutter_test/flutter_test.dart';

import 'package:eyetrack/core/calibration_state.dart';

void main() {
  group('CalibrationPhase', () {
    test('has expected values', () {
      expect(CalibrationPhase.values.length, 6);
      expect(CalibrationPhase.idle, isNotNull);
      expect(CalibrationPhase.instructions, isNotNull);
      expect(CalibrationPhase.collecting, isNotNull);
      expect(CalibrationPhase.processing, isNotNull);
      expect(CalibrationPhase.complete, isNotNull);
      expect(CalibrationPhase.failed, isNotNull);
    });
  });

  group('CalibrationState', () {
    test('default state is idle with 9 points', () {
      const state = CalibrationState();
      expect(state.phase, CalibrationPhase.idle);
      expect(state.currentPointIndex, 0);
      expect(state.totalPoints, 9);
      expect(state.holdProgress, 0.0);
      expect(state.accuracy, isNull);
      expect(state.errorMessage, isNull);
    });

    test('isActive returns true during active phases', () {
      expect(
        const CalibrationState(phase: CalibrationPhase.instructions).isActive,
        isTrue,
      );
      expect(
        const CalibrationState(phase: CalibrationPhase.collecting).isActive,
        isTrue,
      );
      expect(
        const CalibrationState(phase: CalibrationPhase.processing).isActive,
        isTrue,
      );
    });

    test('isActive returns false during inactive phases', () {
      expect(
        const CalibrationState(phase: CalibrationPhase.idle).isActive,
        isFalse,
      );
      expect(
        const CalibrationState(phase: CalibrationPhase.complete).isActive,
        isFalse,
      );
      expect(
        const CalibrationState(phase: CalibrationPhase.failed).isActive,
        isFalse,
      );
    });

    test('isComplete and isFailed', () {
      expect(
        const CalibrationState(phase: CalibrationPhase.complete).isComplete,
        isTrue,
      );
      expect(
        const CalibrationState(phase: CalibrationPhase.failed).isFailed,
        isTrue,
      );
    });

    test('copyWith preserves values', () {
      const original = CalibrationState(
        phase: CalibrationPhase.collecting,
        currentPointIndex: 3,
        holdProgress: 0.5,
      );
      final copied = original.copyWith(holdProgress: 0.8);
      expect(copied.phase, CalibrationPhase.collecting);
      expect(copied.currentPointIndex, 3);
      expect(copied.holdProgress, 0.8);
    });

    test('copyWith changes phase', () {
      const state = CalibrationState();
      final updated = state.copyWith(phase: CalibrationPhase.complete, accuracy: 95.5);
      expect(updated.phase, CalibrationPhase.complete);
      expect(updated.accuracy, 95.5);
    });

    test('defaultPoints has 9 points', () {
      expect(CalibrationState.defaultPoints.length, 9);
    });

    test('defaultPoints are in valid range [0, 1]', () {
      for (final point in CalibrationState.defaultPoints) {
        expect(point.x, inInclusiveRange(0.0, 1.0));
        expect(point.y, inInclusiveRange(0.0, 1.0));
      }
    });

    test('defaultPoints form a 3x3 grid', () {
      final xValues = CalibrationState.defaultPoints.map((p) => p.x).toSet();
      final yValues = CalibrationState.defaultPoints.map((p) => p.y).toSet();
      expect(xValues.length, 3);
      expect(yValues.length, 3);
    });
  });
}
