import 'package:flutter/material.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'package:eyetrack/core/platform_config.dart';
import 'package:eyetrack/core/settings_state.dart';
import 'package:eyetrack/l10n/app_localizations.dart';
import 'package:eyetrack/presentation/screens/settings_screen.dart';

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
  group('SettingsScreen', () {
    testWidgets('shows settings title in app bar', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const SettingsScreen()));
      await tester.pumpAndSettle();

      expect(find.text('Settings'), findsOneWidget);
    });

    testWidgets('shows server URL field', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const SettingsScreen()));
      await tester.pumpAndSettle();

      expect(find.text('Server URL'), findsOneWidget);
      expect(find.text('localhost'), findsOneWidget);
    });

    testWidgets('shows port field', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const SettingsScreen()));
      await tester.pumpAndSettle();

      expect(find.text('Port'), findsOneWidget);
      expect(find.text('50051'), findsOneWidget);
    });

    testWidgets('shows protocol selector', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const SettingsScreen()));
      await tester.pumpAndSettle();

      expect(find.text('Protocol'), findsOneWidget);
      expect(find.text('gRPC'), findsOneWidget);
      expect(find.text('WebSocket'), findsOneWidget);
      expect(find.text('MQTT'), findsOneWidget);
    });

    testWidgets('shows EAR threshold slider', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const SettingsScreen()));
      await tester.pumpAndSettle();

      expect(find.text('EAR Threshold'), findsOneWidget);
      expect(find.byType(Slider), findsOneWidget);
    });

    testWidgets('shows profile chips', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const SettingsScreen()));
      await tester.pumpAndSettle();

      expect(find.text('Profile'), findsOneWidget);
      expect(find.text('Default'), findsOneWidget);
      expect(find.byType(ChoiceChip), findsOneWidget);
    });

    testWidgets('shows save button', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const SettingsScreen()));
      await tester.pumpAndSettle();

      expect(find.text('Save'), findsOneWidget);
    });

    testWidgets('server URL is editable', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const SettingsScreen()));
      await tester.pumpAndSettle();

      final state = tester.state<SettingsScreenState>(
        find.byType(SettingsScreen),
      );

      // Find the server URL text field and enter text
      final serverField = find.widgetWithText(TextField, 'Server URL');
      await tester.enterText(serverField, '192.168.1.100');
      await tester.pump();

      expect(state.state.serverUrl, '192.168.1.100');
    });

    testWidgets('port is editable', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const SettingsScreen()));
      await tester.pumpAndSettle();

      final state = tester.state<SettingsScreenState>(
        find.byType(SettingsScreen),
      );

      final portField = find.widgetWithText(TextField, 'Port');
      await tester.enterText(portField, '8080');
      await tester.pump();

      expect(state.state.port, 8080);
    });

    testWidgets('protocol is selectable', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const SettingsScreen()));
      await tester.pumpAndSettle();

      final state = tester.state<SettingsScreenState>(
        find.byType(SettingsScreen),
      );

      expect(state.state.protocol, Protocol.grpc);

      await tester.tap(find.text('WebSocket'));
      await tester.pump();

      expect(state.state.protocol, Protocol.webSocket);
      expect(state.state.port, 8080);
    });

    testWidgets('EAR threshold slider updates value', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const SettingsScreen()));
      await tester.pumpAndSettle();

      final state = tester.state<SettingsScreenState>(
        find.byType(SettingsScreen),
      );

      expect(state.state.earThreshold, 0.2);

      state.updateEarThreshold(0.3);
      await tester.pump();

      expect(state.state.earThreshold, 0.3);
    });

    testWidgets('profile can be added', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const SettingsScreen()));
      await tester.pumpAndSettle();

      final state = tester.state<SettingsScreenState>(
        find.byType(SettingsScreen),
      );

      state.addProfile('Work');
      await tester.pump();

      expect(state.state.profiles, ['Default', 'Work']);
      expect(state.state.profileName, 'Work');
      expect(find.text('Work'), findsOneWidget);
    });

    testWidgets('duplicate profile is not added', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const SettingsScreen()));
      await tester.pumpAndSettle();

      final state = tester.state<SettingsScreenState>(
        find.byType(SettingsScreen),
      );

      state.addProfile('Default');
      await tester.pump();

      expect(state.state.profiles, ['Default']);
    });

    testWidgets('empty profile name is not added', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const SettingsScreen()));
      await tester.pumpAndSettle();

      final state = tester.state<SettingsScreenState>(
        find.byType(SettingsScreen),
      );

      state.addProfile('');
      await tester.pump();

      expect(state.state.profiles, ['Default']);
    });

    testWidgets('profile can be deleted', (tester) async {
      await tester.pumpWidget(_wrapWithApp(SettingsScreen(
        initialSettings: const SettingsState(
          profiles: ['Default', 'Work'],
          profileName: 'Work',
        ),
      )));
      await tester.pumpAndSettle();

      final state = tester.state<SettingsScreenState>(
        find.byType(SettingsScreen),
      );

      state.deleteProfile('Work');
      await tester.pump();

      expect(state.state.profiles, ['Default']);
      expect(state.state.profileName, 'Default');
    });

    testWidgets('last profile cannot be deleted', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const SettingsScreen()));
      await tester.pumpAndSettle();

      final state = tester.state<SettingsScreenState>(
        find.byType(SettingsScreen),
      );

      state.deleteProfile('Default');
      await tester.pump();

      expect(state.state.profiles, ['Default']);
    });

    testWidgets('profile selection works', (tester) async {
      await tester.pumpWidget(_wrapWithApp(SettingsScreen(
        initialSettings: const SettingsState(
          profiles: ['Default', 'Work'],
          profileName: 'Default',
        ),
      )));
      await tester.pumpAndSettle();

      final state = tester.state<SettingsScreenState>(
        find.byType(SettingsScreen),
      );

      state.selectProfile('Work');
      await tester.pump();

      expect(state.state.profileName, 'Work');
    });

    testWidgets('save button calls onSave with current state', (tester) async {
      SettingsState? saved;
      await tester.pumpWidget(_wrapWithApp(SettingsScreen(
        onSave: (state) => saved = state,
      )));
      await tester.pumpAndSettle();

      final state = tester.state<SettingsScreenState>(
        find.byType(SettingsScreen),
      );

      state.updateServerUrl('example.com');
      await tester.pump();

      await tester.tap(find.text('Save'));
      await tester.pump();

      expect(saved, isNotNull);
      expect(saved!.serverUrl, 'example.com');
    });

    testWidgets('save persists to SharedPreferences', (tester) async {
      SharedPreferences.setMockInitialValues({});
      final prefs = await SharedPreferences.getInstance();

      await tester.pumpWidget(_wrapWithApp(SettingsScreen(
        prefs: prefs,
      )));
      await tester.pumpAndSettle();

      final state = tester.state<SettingsScreenState>(
        find.byType(SettingsScreen),
      );

      state.updateServerUrl('saved-host.com');
      await tester.pump();

      await tester.tap(find.text('Save'));
      await tester.pumpAndSettle();

      expect(prefs.getString('settings_server_url'), 'saved-host.com');
    });

    testWidgets('loads initial settings', (tester) async {
      await tester.pumpWidget(_wrapWithApp(SettingsScreen(
        initialSettings: const SettingsState(
          serverUrl: '10.0.0.1',
          port: 9090,
          protocol: Protocol.mqtt,
          earThreshold: 0.35,
          profileName: 'Custom',
          profiles: ['Default', 'Custom'],
        ),
      )));
      await tester.pumpAndSettle();

      expect(find.text('10.0.0.1'), findsOneWidget);
      expect(find.text('9090'), findsOneWidget);
      expect(find.text('Custom'), findsOneWidget);
    });

    testWidgets('add profile button shows dialog', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const SettingsScreen()));
      await tester.pumpAndSettle();

      await tester.tap(find.text('Add Profile'));
      await tester.pumpAndSettle();

      expect(find.text('Profile Name'), findsOneWidget);
      expect(find.text('Cancel'), findsWidgets);
      expect(find.text('Add'), findsOneWidget);
    });

    testWidgets('delete button hidden with single profile', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const SettingsScreen()));
      await tester.pumpAndSettle();

      expect(find.text('Delete Profile'), findsNothing);
    });

    testWidgets('delete button shown with multiple profiles', (tester) async {
      await tester.pumpWidget(_wrapWithApp(SettingsScreen(
        initialSettings: const SettingsState(
          profiles: ['Default', 'Work'],
        ),
      )));
      await tester.pumpAndSettle();

      expect(find.text('Delete Profile'), findsOneWidget);
    });

    testWidgets('switching protocol updates port automatically', (tester) async {
      await tester.pumpWidget(_wrapWithApp(const SettingsScreen()));
      await tester.pumpAndSettle();

      final state = tester.state<SettingsScreenState>(
        find.byType(SettingsScreen),
      );

      state.updateProtocol(Protocol.mqtt);
      await tester.pump();

      expect(state.state.port, 1883);
      expect(find.text('1883'), findsOneWidget);
    });
  });
}
