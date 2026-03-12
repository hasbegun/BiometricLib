import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:eyetrack/core/camera_service.dart';
import 'package:eyetrack/core/tracking_state.dart';
import 'package:eyetrack/l10n/app_localizations.dart';
import 'package:eyetrack/presentation/screens/tracking_screen.dart';

Widget _wrapWithApp(Widget child) {
  return MaterialApp(
    localizationsDelegates: const [
      AppLocalizations.delegate,
      GlobalMaterialLocalizations.delegate,
      GlobalWidgetsLocalizations.delegate,
      GlobalCupertinoLocalizations.delegate,
    ],
    supportedLocales: AppLocalizations.supportedLocales,
    home: child,
  );
}

void main() {
  setUp(() {
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(
      const MethodChannel('plugins.flutter.io/camera_avfoundation'),
      (MethodCall methodCall) async {
        if (methodCall.method == 'availableCameras') {
          return <Map<String, dynamic>>[];
        }
        return null;
      },
    );
  });

  tearDown(() {
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(
      const MethodChannel('plugins.flutter.io/camera_avfoundation'),
      null,
    );
  });

  group('TrackingScreen', () {
    testWidgets('shows app bar with title', (tester) async {
      await tester.pumpWidget(_wrapWithApp(
        TrackingScreen(cameraService: CameraService()),
      ));
      await tester.pump();
      await tester.pump();

      expect(find.text('Eye Tracking'), findsOneWidget);
    });

    testWidgets('shows 8x8 grid', (tester) async {
      await tester.pumpWidget(_wrapWithApp(
        TrackingScreen(cameraService: CameraService()),
      ));
      await tester.pump();
      await tester.pump();

      expect(find.byType(GridView), findsOneWidget);
      // First row tiles should be visible
      expect(find.text('1'), findsOneWidget);
      expect(find.text('8'), findsOneWidget);
    });

    testWidgets('shows "No gaze detected" when not tracking', (tester) async {
      await tester.pumpWidget(_wrapWithApp(
        TrackingScreen(cameraService: CameraService()),
      ));
      await tester.pump();
      await tester.pump();

      expect(find.text('No gaze detected'), findsOneWidget);
    });

    testWidgets('shows start tracking button initially', (tester) async {
      await tester.pumpWidget(_wrapWithApp(
        TrackingScreen(cameraService: CameraService()),
      ));
      await tester.pump();
      await tester.pump();

      expect(find.text('Start Tracking'), findsOneWidget);
      expect(find.byIcon(Icons.play_arrow), findsOneWidget);
    });

    testWidgets('toggles to stop tracking on button press', (tester) async {
      await tester.pumpWidget(_wrapWithApp(
        TrackingScreen(cameraService: CameraService()),
      ));
      await tester.pump();
      await tester.pump();

      await tester.tap(find.text('Start Tracking'));
      await tester.pump();

      expect(find.text('Stop Tracking'), findsOneWidget);
      expect(find.byIcon(Icons.stop), findsOneWidget);
    });

    testWidgets('shows tile number when tracking', (tester) async {
      await tester.pumpWidget(_wrapWithApp(
        TrackingScreen(cameraService: CameraService()),
      ));
      await tester.pump();
      await tester.pump();

      final state = tester.state<TrackingScreenState>(
        find.byType(TrackingScreen),
      );

      state.setTracking(true);
      await tester.pump();

      // Default gaze is (0.5, 0.5) → col=4, row=4 → tile 37
      expect(find.text('Tile: 37'), findsOneWidget);
    });

    testWidgets('gazeTile computes correctly for different positions', (tester) async {
      await tester.pumpWidget(_wrapWithApp(
        TrackingScreen(cameraService: CameraService()),
      ));
      await tester.pump();
      await tester.pump();

      final state = tester.state<TrackingScreenState>(
        find.byType(TrackingScreen),
      );

      // Top-left corner: (0.0, 0.0) → tile 1
      state.updateGaze(0.0, 0.0, 0.9);
      expect(state.gazeTile, 1);

      // Top-right corner: (0.99, 0.0) → col=7, row=0 → tile 8
      state.updateGaze(0.99, 0.0, 0.9);
      expect(state.gazeTile, 8);

      // Bottom-left corner: (0.0, 0.99) → col=0, row=7 → tile 57
      state.updateGaze(0.0, 0.99, 0.9);
      expect(state.gazeTile, 57);

      // Bottom-right corner: (0.99, 0.99) → col=7, row=7 → tile 64
      state.updateGaze(0.99, 0.99, 0.9);
      expect(state.gazeTile, 64);

      // Center: (0.5, 0.5) → col=4, row=4 → tile 37
      state.updateGaze(0.5, 0.5, 0.9);
      expect(state.gazeTile, 37);
    });

    testWidgets('calls onStartTracking callback', (tester) async {
      var started = false;
      await tester.pumpWidget(_wrapWithApp(TrackingScreen(
        cameraService: CameraService(),
        onStartTracking: () => started = true,
      )));
      await tester.pump();
      await tester.pump();

      await tester.tap(find.text('Start Tracking'));
      await tester.pump();

      expect(started, isTrue);
    });

    testWidgets('calls onStopTracking callback', (tester) async {
      var stopped = false;
      await tester.pumpWidget(_wrapWithApp(TrackingScreen(
        cameraService: CameraService(),
        onStopTracking: () => stopped = true,
      )));
      await tester.pump();
      await tester.pump();

      await tester.tap(find.text('Start Tracking'));
      await tester.pump();
      await tester.tap(find.text('Stop Tracking'));
      await tester.pump();

      expect(stopped, isTrue);
    });

    testWidgets('does not highlight tile when not tracking', (tester) async {
      await tester.pumpWidget(_wrapWithApp(
        TrackingScreen(cameraService: CameraService()),
      ));
      await tester.pump();
      await tester.pump();

      final state = tester.state<TrackingScreenState>(
        find.byType(TrackingScreen),
      );
      expect(state.state.isTracking, false);
    });

    testWidgets('updateGaze changes gaze state', (tester) async {
      await tester.pumpWidget(_wrapWithApp(
        TrackingScreen(cameraService: CameraService()),
      ));
      await tester.pump();
      await tester.pump();

      final state = tester.state<TrackingScreenState>(
        find.byType(TrackingScreen),
      );

      state.setTracking(true);
      state.updateGaze(0.3, 0.7, 0.95);
      await tester.pump();

      expect(state.state.gazeX, 0.3);
      expect(state.state.gazeY, 0.7);
      expect(state.state.confidence, 0.95);
      // col=2, row=5 → tile 43
      expect(state.gazeTile, 43);
    });

    testWidgets('shows single blink indicator', (tester) async {
      await tester.pumpWidget(_wrapWithApp(
        TrackingScreen(cameraService: CameraService()),
      ));
      await tester.pump();
      await tester.pump();

      final state = tester.state<TrackingScreenState>(
        find.byType(TrackingScreen),
      );

      state.updateBlink(BlinkIndicator.single);
      await tester.pump();

      expect(find.text('Single Blink'), findsOneWidget);

      await tester.pump(const Duration(milliseconds: 500));
    });

    testWidgets('shows double blink indicator', (tester) async {
      await tester.pumpWidget(_wrapWithApp(
        TrackingScreen(cameraService: CameraService()),
      ));
      await tester.pump();
      await tester.pump();

      final state = tester.state<TrackingScreenState>(
        find.byType(TrackingScreen),
      );

      state.updateBlink(BlinkIndicator.double_);
      await tester.pump();

      expect(find.text('Double Blink'), findsOneWidget);

      await tester.pump(const Duration(milliseconds: 500));
    });

    testWidgets('shows long blink indicator', (tester) async {
      await tester.pumpWidget(_wrapWithApp(
        TrackingScreen(cameraService: CameraService()),
      ));
      await tester.pump();
      await tester.pump();

      final state = tester.state<TrackingScreenState>(
        find.byType(TrackingScreen),
      );

      state.updateBlink(BlinkIndicator.long);
      await tester.pump();

      expect(find.text('Long Blink'), findsOneWidget);

      await tester.pump(const Duration(milliseconds: 500));
    });

    testWidgets('debug mode toggle shows debug icon', (tester) async {
      await tester.pumpWidget(_wrapWithApp(
        TrackingScreen(cameraService: CameraService()),
      ));
      await tester.pump();
      await tester.pump();

      expect(find.byIcon(Icons.bug_report_outlined), findsOneWidget);
    });

    testWidgets('debug mode shows overlay with FPS, latency, and tile', (tester) async {
      await tester.pumpWidget(_wrapWithApp(
        TrackingScreen(cameraService: CameraService()),
      ));
      await tester.pump();
      await tester.pump();

      final state = tester.state<TrackingScreenState>(
        find.byType(TrackingScreen),
      );

      state.toggleDebugMode();
      state.updateDebugInfo(fps: 29.5, latencyMs: 12.3, earLeft: 0.31, earRight: 0.29);
      await tester.pump();

      expect(find.textContaining('29.5'), findsOneWidget);
      expect(find.textContaining('12.3'), findsOneWidget);
      expect(find.textContaining('0.310'), findsOneWidget);
      expect(find.textContaining('0.290'), findsOneWidget);
      expect(find.textContaining('Tile:'), findsOneWidget);
      expect(find.byType(LinearProgressIndicator), findsOneWidget);
    });

    testWidgets('debug overlay hidden when debug mode off', (tester) async {
      await tester.pumpWidget(_wrapWithApp(
        TrackingScreen(cameraService: CameraService()),
      ));
      await tester.pump();
      await tester.pump();

      final state = tester.state<TrackingScreenState>(
        find.byType(TrackingScreen),
      );

      expect(state.state.debugMode, false);
      expect(find.byType(LinearProgressIndicator), findsNothing);
    });

    testWidgets('toggling debug mode twice hides overlay', (tester) async {
      await tester.pumpWidget(_wrapWithApp(
        TrackingScreen(cameraService: CameraService()),
      ));
      await tester.pump();
      await tester.pump();

      final state = tester.state<TrackingScreenState>(
        find.byType(TrackingScreen),
      );

      state.toggleDebugMode();
      await tester.pump();
      expect(state.state.debugMode, true);

      state.toggleDebugMode();
      await tester.pump();
      expect(state.state.debugMode, false);
      expect(find.byType(LinearProgressIndicator), findsNothing);
    });

    testWidgets('debug toggle button changes icon', (tester) async {
      await tester.pumpWidget(_wrapWithApp(
        TrackingScreen(cameraService: CameraService()),
      ));
      await tester.pump();
      await tester.pump();

      expect(find.byIcon(Icons.bug_report_outlined), findsOneWidget);
      expect(find.byIcon(Icons.bug_report), findsNothing);

      await tester.tap(find.byIcon(Icons.bug_report_outlined));
      await tester.pump();

      expect(find.byIcon(Icons.bug_report), findsOneWidget);
    });

    testWidgets('tile display updates when gaze changes', (tester) async {
      await tester.pumpWidget(_wrapWithApp(
        TrackingScreen(cameraService: CameraService()),
      ));
      await tester.pump();
      await tester.pump();

      final state = tester.state<TrackingScreenState>(
        find.byType(TrackingScreen),
      );

      state.setTracking(true);
      await tester.pump();

      // Default gaze (0.5, 0.5) → tile 37
      expect(find.text('Tile: 37'), findsOneWidget);

      // Move gaze to top-left → tile 1
      state.updateGaze(0.05, 0.05, 0.8);
      await tester.pump();

      expect(find.text('Tile: 1'), findsOneWidget);
    });
  });
}
