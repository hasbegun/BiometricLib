import 'package:flutter/foundation.dart';

/// Stores calibration mapping from raw iris ratios to screen coordinates.
///
/// During calibration, the user looks at known screen points while we record
/// the corresponding iris ratios. This gives us the actual range of iris
/// movement for this user, which we then map to the full 0.0-1.0 screen range.
@immutable
class GazeCalibrationData {
  const GazeCalibrationData({
    required this.minIrisH,
    required this.maxIrisH,
    required this.minIrisV,
    required this.maxIrisV,
  });

  /// Iris ratio when looking at the right edge of screen (irisH is low when looking right in camera).
  final double minIrisH;

  /// Iris ratio when looking at the left edge of screen (irisH is high when looking left in camera).
  final double maxIrisH;

  /// Iris ratio when looking at the top of screen.
  final double minIrisV;

  /// Iris ratio when looking at the bottom of screen.
  final double maxIrisV;

  double get rangeH => (maxIrisH - minIrisH).abs();
  double get rangeV => (maxIrisV - minIrisV).abs();
  bool get isValid => rangeH > 0.02 && rangeV > 0.02;

  /// Map a raw iris ratio to calibrated screen coordinate (0.0-1.0).
  double mapH(double rawIrisH) {
    if (rangeH < 0.01) return 0.5;
    // Invert: high irisH = looking left in camera = left on screen = low gazeX
    final normalized = (rawIrisH - minIrisH) / (maxIrisH - minIrisH);
    return (1.0 - normalized).clamp(0.0, 1.0);
  }

  double mapV(double rawIrisV) {
    if (rangeV < 0.01) return 0.5;
    final normalized = (rawIrisV - minIrisV) / (maxIrisV - minIrisV);
    return normalized.clamp(0.0, 1.0);
  }

  /// Build calibration data from collected samples at known screen points.
  ///
  /// [samples] is a list of (screenX, screenY, irisH, irisV) tuples collected
  /// while the user looked at calibration points.
  factory GazeCalibrationData.fromSamples(List<CalibrationSample> samples) {
    if (samples.isEmpty) {
      return const GazeCalibrationData(
        minIrisH: 0.35, maxIrisH: 0.65,
        minIrisV: 0.35, maxIrisV: 0.65,
      );
    }

    // Find iris ratios at extreme screen positions
    // Left screen edge (screenX ≈ 0): highest irisH (looking left in camera)
    // Right screen edge (screenX ≈ 1): lowest irisH
    // Top screen edge (screenY ≈ 0): lowest irisV
    // Bottom screen edge (screenY ≈ 1): highest irisV

    final leftSamples = samples.where((s) => s.screenX < 0.3).toList();
    final rightSamples = samples.where((s) => s.screenX > 0.7).toList();
    final topSamples = samples.where((s) => s.screenY < 0.3).toList();
    final bottomSamples = samples.where((s) => s.screenY > 0.7).toList();

    double avgIrisH(List<CalibrationSample> list) {
      if (list.isEmpty) return 0.5;
      return list.map((s) => s.irisH).reduce((a, b) => a + b) / list.length;
    }
    double avgIrisV(List<CalibrationSample> list) {
      if (list.isEmpty) return 0.5;
      return list.map((s) => s.irisV).reduce((a, b) => a + b) / list.length;
    }

    // maxIrisH = when looking left (screen left), minIrisH = when looking right (screen right)
    final maxH = avgIrisH(leftSamples);
    final minH = avgIrisH(rightSamples);
    final minV = avgIrisV(topSamples);
    final maxV = avgIrisV(bottomSamples);

    // Add 10% margin to extend the range slightly beyond calibration points
    final marginH = (maxH - minH).abs() * 0.1;
    final marginV = (maxV - minV).abs() * 0.1;

    return GazeCalibrationData(
      minIrisH: minH - marginH,
      maxIrisH: maxH + marginH,
      minIrisV: minV - marginV,
      maxIrisV: maxV + marginV,
    );
  }

  @override
  String toString() =>
      'GazeCalibrationData(H: ${minIrisH.toStringAsFixed(3)}-${maxIrisH.toStringAsFixed(3)}, '
      'V: ${minIrisV.toStringAsFixed(3)}-${maxIrisV.toStringAsFixed(3)})';
}

/// A single calibration sample: screen point + observed iris ratios.
@immutable
class CalibrationSample {
  const CalibrationSample({
    required this.screenX,
    required this.screenY,
    required this.irisH,
    required this.irisV,
  });

  final double screenX;
  final double screenY;
  final double irisH;
  final double irisV;
}
