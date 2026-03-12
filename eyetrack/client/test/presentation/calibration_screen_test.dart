import 'package:flutter/material.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:eyetrack/core/calibration_state.dart';
import 'package:eyetrack/l10n/app_localizations.dart';
import 'package:eyetrack/presentation/screens/calibration_screen.dart';

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
  group('CalibrationScreen', () {
    testWidgets('shows instructions on initial render', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const CalibrationScreen()));
      await tester.pumpAndSettle();

      // Should show calibration title in app bar
      expect(find.text('Calibration'), findsOneWidget);
      // Should show instructions text
      expect(
        find.text('Look at each point as it appears on the screen.'),
        findsOneWidget,
      );
      // Should show start button
      expect(find.text('Calibrate'), findsOneWidget);
    });

    testWidgets('has close button for cancel', (tester) async {
      var cancelled = false;
      await tester.pumpWidget(_wrapWithApp(CalibrationScreen(
        onCancel: () => cancelled = true,
      )));
      await tester.pumpAndSettle();

      expect(find.byIcon(Icons.close), findsOneWidget);
      await tester.tap(find.byIcon(Icons.close));
      expect(cancelled, isTrue);
    });

    testWidgets('starts calibration when button pressed', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const CalibrationScreen()));
      await tester.pumpAndSettle();

      await tester.tap(find.text('Calibrate'));
      await tester.pump();

      // Should show point counter
      expect(find.text('1 / 9'), findsOneWidget);
      // Should show progress indicator
      expect(find.byType(LinearProgressIndicator), findsOneWidget);
      expect(find.byType(CircularProgressIndicator), findsOneWidget);
    });

    testWidgets('shows 9 points during calibration', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const CalibrationScreen()));
      await tester.pumpAndSettle();

      // Access state to verify totalPoints
      final state = tester.state<CalibrationScreenState>(
        find.byType(CalibrationScreen),
      );

      state.startCalibration();
      await tester.pump();

      expect(state.state.totalPoints, 9);
      expect(state.state.currentPointIndex, 0);
    });

    testWidgets('hold progress animation runs for 2 seconds', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const CalibrationScreen(
        holdDuration: Duration(seconds: 2),
      )));
      await tester.pumpAndSettle();

      final state = tester.state<CalibrationScreenState>(
        find.byType(CalibrationScreen),
      );

      state.startCalibration();
      await tester.pump();

      // Pump in small increments to let the animation controller advance
      for (var i = 0; i < 10; i++) {
        await tester.pump(const Duration(milliseconds: 100));
      }
      // After ~1 second, progress should be around 50%
      expect(state.state.holdProgress, closeTo(0.5, 0.15));

      // Pump the remaining second
      for (var i = 0; i < 11; i++) {
        await tester.pump(const Duration(milliseconds: 100));
      }
      // Should have advanced to next point
      expect(state.state.currentPointIndex, 1);
    });

    testWidgets('advances through all 9 points', (tester) async {
      final collected = <({double x, double y})>[];
      await tester.pumpWidget(_wrapWithApp(CalibrationScreen(
        holdDuration: const Duration(milliseconds: 100),
        onCalibrationComplete: (points, calData) => collected.addAll(points),
      )));
      await tester.pumpAndSettle();

      final state = tester.state<CalibrationScreenState>(
        find.byType(CalibrationScreen),
      );

      state.startCalibration();
      await tester.pump(); // Initial frame

      // Advance through all 9 points
      for (var i = 0; i < 9; i++) {
        // Pump past the hold duration to complete each animation
        await tester.pump(const Duration(milliseconds: 120));
        await tester.pump(); // Process setState
      }

      expect(state.state.phase, CalibrationPhase.processing);

      // Wait for Future.delayed(500ms) that fires the callback
      await tester.pump(const Duration(milliseconds: 600));
      await tester.pump();

      expect(collected.length, 9);
    });

    testWidgets('calls onCalibrationComplete with collected points', (tester) async {
      List<({double x, double y})>? result;
      await tester.pumpWidget(_wrapWithApp(CalibrationScreen(
        holdDuration: const Duration(milliseconds: 50),
        onCalibrationComplete: (points, calData) => result = points,
      )));
      await tester.pumpAndSettle();

      final state = tester.state<CalibrationScreenState>(
        find.byType(CalibrationScreen),
      );

      state.startCalibration();
      await tester.pump();
      for (var i = 0; i < 9; i++) {
        await tester.pump(const Duration(milliseconds: 60));
        await tester.pump();
      }

      // Wait for Future.delayed(500ms) that fires the callback
      await tester.pump(const Duration(milliseconds: 600));
      await tester.pump();

      expect(result, isNotNull);
      expect(result!.length, 9);
      // First point should match defaultPoints[0]
      expect(result![0].x, CalibrationState.defaultPoints[0].x);
      expect(result![0].y, CalibrationState.defaultPoints[0].y);
    });

    testWidgets('shows success view with accuracy', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const CalibrationScreen()));
      await tester.pumpAndSettle();

      final state = tester.state<CalibrationScreenState>(
        find.byType(CalibrationScreen),
      );

      state.setComplete(95.5);
      await tester.pump();

      expect(find.text('Calibration complete!'), findsOneWidget);
      expect(find.byIcon(Icons.check_circle), findsOneWidget);
    });

    testWidgets('shows failure view with retry button', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const CalibrationScreen()));
      await tester.pumpAndSettle();

      final state = tester.state<CalibrationScreenState>(
        find.byType(CalibrationScreen),
      );

      state.setFailed('Connection lost');
      await tester.pump();

      expect(find.text('Connection lost'), findsOneWidget);
      expect(find.byIcon(Icons.error), findsOneWidget);
      expect(find.text('Retry'), findsOneWidget);
    });

    testWidgets('retry returns to instructions', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const CalibrationScreen()));
      await tester.pumpAndSettle();

      final state = tester.state<CalibrationScreenState>(
        find.byType(CalibrationScreen),
      );

      state.setFailed('Error');
      await tester.pump();

      await tester.tap(find.text('Retry'));
      await tester.pump();

      expect(state.state.phase, CalibrationPhase.instructions);
      expect(
        find.text('Look at each point as it appears on the screen.'),
        findsOneWidget,
      );
    });

    testWidgets('cancel stops calibration mid-way', (tester) async {
      var cancelled = false;
      await tester.pumpWidget(_wrapWithApp(CalibrationScreen(
        holdDuration: const Duration(seconds: 2),
        onCancel: () => cancelled = true,
      )));
      await tester.pumpAndSettle();

      final state = tester.state<CalibrationScreenState>(
        find.byType(CalibrationScreen),
      );

      state.startCalibration();
      await tester.pump(const Duration(seconds: 1));

      state.cancel();
      await tester.pump();

      expect(cancelled, isTrue);
      expect(state.state.phase, CalibrationPhase.idle);
    });
  });
}
