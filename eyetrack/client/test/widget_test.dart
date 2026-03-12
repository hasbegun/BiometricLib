import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:eyetrack/main.dart';

void main() {
  setUp(() {
    // Mock the camera platform channel to return empty camera list
    // so availableCameras() doesn't hang in tests
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

  testWidgets('App renders with home screen title', (WidgetTester tester) async {
    await tester.pumpWidget(const ProviderScope(child: EyetrackApp()));
    // Use pump() instead of pumpAndSettle() since camera init may not settle
    await tester.pump();
    await tester.pump();

    // The app should render with navigation and tracking screen visible
    expect(find.byType(Scaffold), findsWidgets);
    expect(find.text('Eye Tracking'), findsWidgets);
  });

  testWidgets('App uses Material 3', (WidgetTester tester) async {
    await tester.pumpWidget(const ProviderScope(child: EyetrackApp()));
    await tester.pump();

    final materialApp = tester.widget<MaterialApp>(find.byType(MaterialApp));
    expect(materialApp.theme?.useMaterial3, isTrue);
  });

  testWidgets('App has localization delegates configured', (WidgetTester tester) async {
    await tester.pumpWidget(const ProviderScope(child: EyetrackApp()));
    await tester.pump();

    final materialApp = tester.widget<MaterialApp>(find.byType(MaterialApp));
    expect(materialApp.localizationsDelegates, isNotNull);
    expect(materialApp.supportedLocales, isNotEmpty);
  });

  testWidgets('App has bottom navigation bar', (WidgetTester tester) async {
    await tester.pumpWidget(const ProviderScope(child: EyetrackApp()));
    await tester.pump();
    await tester.pump();

    expect(find.byType(NavigationBar), findsOneWidget);
    // First tab is selected so it shows the filled icon, second shows outlined
    expect(find.byIcon(Icons.visibility), findsOneWidget);
    expect(find.byIcon(Icons.settings_outlined), findsOneWidget);
  });

  testWidgets('App has dark theme configured', (WidgetTester tester) async {
    await tester.pumpWidget(const ProviderScope(child: EyetrackApp()));
    await tester.pump();

    final materialApp = tester.widget<MaterialApp>(find.byType(MaterialApp));
    expect(materialApp.darkTheme, isNotNull);
    expect(materialApp.darkTheme?.useMaterial3, isTrue);
  });

  testWidgets('Navigation switches to settings tab', (WidgetTester tester) async {
    await tester.pumpWidget(const ProviderScope(child: EyetrackApp()));
    await tester.pump();
    await tester.pump();

    // Tap Settings tab
    await tester.tap(find.byIcon(Icons.settings_outlined));
    await tester.pump();
    await tester.pump();

    // Settings screen fields should be visible
    expect(find.text('Server URL'), findsOneWidget);
    expect(find.text('Protocol'), findsOneWidget);
  });
}
