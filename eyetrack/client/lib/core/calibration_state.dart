import 'package:flutter/foundation.dart';

enum CalibrationPhase {
  idle,
  instructions,
  collecting,
  processing,
  complete,
  failed,
}

@immutable
class CalibrationState {
  const CalibrationState({
    this.phase = CalibrationPhase.idle,
    this.currentPointIndex = 0,
    this.totalPoints = 9,
    this.holdProgress = 0.0,
    this.accuracy,
    this.errorMessage,
  });

  final CalibrationPhase phase;
  final int currentPointIndex;
  final int totalPoints;
  final double holdProgress; // 0.0 to 1.0
  final double? accuracy;
  final String? errorMessage;

  bool get isActive =>
      phase == CalibrationPhase.instructions ||
      phase == CalibrationPhase.collecting ||
      phase == CalibrationPhase.processing;

  bool get isComplete => phase == CalibrationPhase.complete;
  bool get isFailed => phase == CalibrationPhase.failed;

  CalibrationState copyWith({
    CalibrationPhase? phase,
    int? currentPointIndex,
    int? totalPoints,
    double? holdProgress,
    double? accuracy,
    String? errorMessage,
  }) {
    return CalibrationState(
      phase: phase ?? this.phase,
      currentPointIndex: currentPointIndex ?? this.currentPointIndex,
      totalPoints: totalPoints ?? this.totalPoints,
      holdProgress: holdProgress ?? this.holdProgress,
      accuracy: accuracy ?? this.accuracy,
      errorMessage: errorMessage ?? this.errorMessage,
    );
  }

  static const List<({double x, double y})> defaultPoints = [
    (x: 0.1, y: 0.1),
    (x: 0.5, y: 0.1),
    (x: 0.9, y: 0.1),
    (x: 0.1, y: 0.5),
    (x: 0.5, y: 0.5),
    (x: 0.9, y: 0.5),
    (x: 0.1, y: 0.9),
    (x: 0.5, y: 0.9),
    (x: 0.9, y: 0.9),
  ];
}
