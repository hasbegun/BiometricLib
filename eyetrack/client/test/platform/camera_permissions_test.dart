import 'dart:io';

import 'package:flutter_test/flutter_test.dart';

void main() {
  group('iOS camera permissions', () {
    test('Info.plist contains NSCameraUsageDescription', () {
      final plist = File('ios/Runner/Info.plist');
      if (!plist.existsSync()) {
        // Skip if not running from client directory
        return;
      }
      final content = plist.readAsStringSync();
      expect(content, contains('NSCameraUsageDescription'));
      expect(content, contains('camera'));
    });

    test('Info.plist has app display name EyeTrack', () {
      final plist = File('ios/Runner/Info.plist');
      if (!plist.existsSync()) return;
      final content = plist.readAsStringSync();
      expect(content, contains('EyeTrack'));
    });
  });

  group('Android camera permissions', () {
    test('AndroidManifest.xml contains CAMERA permission', () {
      final manifest = File('android/app/src/main/AndroidManifest.xml');
      if (!manifest.existsSync()) return;
      final content = manifest.readAsStringSync();
      expect(content, contains('android.permission.CAMERA'));
    });

    test('AndroidManifest.xml has camera feature declarations', () {
      final manifest = File('android/app/src/main/AndroidManifest.xml');
      if (!manifest.existsSync()) return;
      final content = manifest.readAsStringSync();
      expect(content, contains('android.hardware.camera'));
      expect(content, contains('android.hardware.camera.front'));
    });

    test('Camera features are not required (supports devices without cameras)', () {
      final manifest = File('android/app/src/main/AndroidManifest.xml');
      if (!manifest.existsSync()) return;
      final content = manifest.readAsStringSync();
      expect(content, contains('android:required="false"'));
    });

    test('AndroidManifest.xml has app label eyetrack', () {
      final manifest = File('android/app/src/main/AndroidManifest.xml');
      if (!manifest.existsSync()) return;
      final content = manifest.readAsStringSync();
      expect(content, contains('android:label="eyetrack"'));
    });
  });
}
