import 'package:flutter/foundation.dart';

import 'package:eyetrack/generated/eyetrack.pb.dart';

enum BlinkIndicator { none, single, double_, long }

@immutable
class TrackingState {
  const TrackingState({
    this.gazeX = 0.5,
    this.gazeY = 0.5,
    this.confidence = 0.0,
    this.blink = BlinkIndicator.none,
    this.fps = 0.0,
    this.latencyMs = 0.0,
    this.earLeft = 0.0,
    this.earRight = 0.0,
    this.isTracking = false,
    this.debugMode = false,
  });

  final double gazeX;
  final double gazeY;
  final double confidence;
  final BlinkIndicator blink;
  final double fps;
  final double latencyMs;
  final double earLeft;
  final double earRight;
  final bool isTracking;
  final bool debugMode;

  double get earAverage => (earLeft + earRight) / 2.0;

  TrackingState copyWith({
    double? gazeX,
    double? gazeY,
    double? confidence,
    BlinkIndicator? blink,
    double? fps,
    double? latencyMs,
    double? earLeft,
    double? earRight,
    bool? isTracking,
    bool? debugMode,
  }) {
    return TrackingState(
      gazeX: gazeX ?? this.gazeX,
      gazeY: gazeY ?? this.gazeY,
      confidence: confidence ?? this.confidence,
      blink: blink ?? this.blink,
      fps: fps ?? this.fps,
      latencyMs: latencyMs ?? this.latencyMs,
      earLeft: earLeft ?? this.earLeft,
      earRight: earRight ?? this.earRight,
      isTracking: isTracking ?? this.isTracking,
      debugMode: debugMode ?? this.debugMode,
    );
  }

  static BlinkIndicator blinkFromProto(BlinkType type) {
    switch (type) {
      case BlinkType.BLINK_NONE:
        return BlinkIndicator.none;
      case BlinkType.BLINK_SINGLE:
        return BlinkIndicator.single;
      case BlinkType.BLINK_DOUBLE:
        return BlinkIndicator.double_;
      case BlinkType.BLINK_LONG:
        return BlinkIndicator.long;
      default:
        return BlinkIndicator.none;
    }
  }
}
