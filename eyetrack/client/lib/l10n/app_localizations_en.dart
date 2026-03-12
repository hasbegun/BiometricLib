// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for English (`en`).
class AppLocalizationsEn extends AppLocalizations {
  AppLocalizationsEn([String locale = 'en']) : super(locale);

  @override
  String get appTitle => 'EyeTrack';

  @override
  String get homeTitle => 'Eye Tracking';

  @override
  String get calibrate => 'Calibrate';

  @override
  String get startTracking => 'Start Tracking';

  @override
  String get stopTracking => 'Stop Tracking';

  @override
  String get settings => 'Settings';

  @override
  String get serverUrl => 'Server URL';

  @override
  String get protocol => 'Protocol';

  @override
  String get protocolGrpc => 'gRPC';

  @override
  String get protocolWebSocket => 'WebSocket';

  @override
  String get protocolMqtt => 'MQTT';

  @override
  String get connected => 'Connected';

  @override
  String get disconnected => 'Disconnected';

  @override
  String get connecting => 'Connecting...';

  @override
  String get calibrationTitle => 'Calibration';

  @override
  String get calibrationInstructions =>
      'Look at each point as it appears on the screen.';

  @override
  String get calibrationComplete => 'Calibration complete!';

  @override
  String get calibrationFailed => 'Calibration failed. Please try again.';

  @override
  String accuracy(String value) {
    return 'Accuracy: $value%';
  }

  @override
  String fps(String value) {
    return 'FPS: $value';
  }

  @override
  String latency(String value) {
    return 'Latency: ${value}ms';
  }

  @override
  String get blinkSingle => 'Single Blink';

  @override
  String get blinkDouble => 'Double Blink';

  @override
  String get blinkLong => 'Long Blink';

  @override
  String get debugMode => 'Debug Mode';

  @override
  String get earThreshold => 'EAR Threshold';

  @override
  String get cameraPermissionRequired =>
      'Camera permission is required for eye tracking.';

  @override
  String get noCameraAvailable => 'No camera available on this device.';

  @override
  String get language => 'Language';

  @override
  String get english => 'English';

  @override
  String get korean => 'Korean';

  @override
  String get cancel => 'Cancel';

  @override
  String get save => 'Save';

  @override
  String get error => 'Error';

  @override
  String get retry => 'Retry';

  @override
  String get connectionError => 'Could not connect to the server.';

  @override
  String get port => 'Port';

  @override
  String get profile => 'Profile';

  @override
  String get addProfile => 'Add Profile';

  @override
  String get deleteProfile => 'Delete Profile';

  @override
  String get profileName => 'Profile Name';

  @override
  String get add => 'Add';

  @override
  String get delete => 'Delete';

  @override
  String gazeTile(int number) {
    return 'Tile: $number';
  }

  @override
  String get noGaze => 'No gaze detected';
}
