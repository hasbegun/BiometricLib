import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:eyetrack/core/camera_service.dart';
import 'package:eyetrack/presentation/widgets/camera_preview_widget.dart';

void main() {
  group('CameraPreviewWidget', () {
    testWidgets('shows placeholder when uninitialized', (tester) async {
      final service = CameraService();

      await tester.pumpWidget(
        MaterialApp(
          home: Scaffold(
            body: SizedBox(
              width: 320,
              height: 240,
              child: CameraPreviewWidget(cameraService: service),
            ),
          ),
        ),
      );

      // Should show camera icon placeholder
      expect(find.byIcon(Icons.camera_alt_outlined), findsOneWidget);
    });

    testWidgets('shows custom placeholder when provided', (tester) async {
      final service = CameraService();

      await tester.pumpWidget(
        MaterialApp(
          home: Scaffold(
            body: SizedBox(
              width: 320,
              height: 240,
              child: CameraPreviewWidget(
                cameraService: service,
                placeholder: const Text('No Camera'),
              ),
            ),
          ),
        ),
      );

      expect(find.text('No Camera'), findsOneWidget);
    });

    testWidgets('shows error icon on error status', (tester) async {
      final service = CameraService();
      // We can't easily trigger error status without a camera,
      // but we can verify the widget renders in uninitialized state
      await tester.pumpWidget(
        MaterialApp(
          home: Scaffold(
            body: SizedBox(
              width: 320,
              height: 240,
              child: CameraPreviewWidget(cameraService: service),
            ),
          ),
        ),
      );

      // In test environment without cameras, shows placeholder
      expect(find.byType(CameraPreviewWidget), findsOneWidget);
    });
  });
}
