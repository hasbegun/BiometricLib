import 'package:flutter_test/flutter_test.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'package:eyetrack/core/platform_config.dart';
import 'package:eyetrack/core/settings_state.dart';

void main() {
  group('SettingsState', () {
    test('default values', () {
      const state = SettingsState();
      expect(state.serverUrl, 'localhost');
      expect(state.port, 50051);
      expect(state.protocol, Protocol.grpc);
      expect(state.earThreshold, 0.2);
      expect(state.profileName, 'Default');
      expect(state.profiles, ['Default']);
    });

    test('copyWith preserves unchanged fields', () {
      const original = SettingsState(
        serverUrl: '192.168.1.1',
        port: 9090,
        protocol: Protocol.webSocket,
        earThreshold: 0.3,
        profileName: 'Custom',
        profiles: ['Default', 'Custom'],
      );
      final copied = original.copyWith(serverUrl: '10.0.0.1');
      expect(copied.serverUrl, '10.0.0.1');
      expect(copied.port, 9090);
      expect(copied.protocol, Protocol.webSocket);
      expect(copied.earThreshold, 0.3);
      expect(copied.profileName, 'Custom');
      expect(copied.profiles, ['Default', 'Custom']);
    });

    test('copyWith updates multiple fields', () {
      const state = SettingsState();
      final updated = state.copyWith(
        serverUrl: 'example.com',
        port: 8080,
        protocol: Protocol.webSocket,
      );
      expect(updated.serverUrl, 'example.com');
      expect(updated.port, 8080);
      expect(updated.protocol, Protocol.webSocket);
    });

    test('defaultPortForProtocol returns correct ports', () {
      expect(
        const SettingsState(protocol: Protocol.grpc).defaultPortForProtocol,
        50051,
      );
      expect(
        const SettingsState(protocol: Protocol.webSocket).defaultPortForProtocol,
        8080,
      );
      expect(
        const SettingsState(protocol: Protocol.mqtt).defaultPortForProtocol,
        1883,
      );
    });
  });

  group('SettingsState persistence', () {
    test('save and load roundtrip', () async {
      SharedPreferences.setMockInitialValues({});
      final prefs = await SharedPreferences.getInstance();

      const original = SettingsState(
        serverUrl: '10.0.0.5',
        port: 9999,
        protocol: Protocol.mqtt,
        earThreshold: 0.35,
        profileName: 'Test',
        profiles: ['Default', 'Test'],
      );

      await original.save(prefs);
      final loaded = SettingsState.load(prefs);

      expect(loaded.serverUrl, '10.0.0.5');
      expect(loaded.port, 9999);
      expect(loaded.protocol, Protocol.mqtt);
      expect(loaded.earThreshold, closeTo(0.35, 0.001));
      expect(loaded.profileName, 'Test');
      expect(loaded.profiles, ['Default', 'Test']);
    });

    test('load with empty prefs returns defaults', () async {
      SharedPreferences.setMockInitialValues({});
      final prefs = await SharedPreferences.getInstance();

      final loaded = SettingsState.load(prefs);
      expect(loaded.serverUrl, 'localhost');
      expect(loaded.port, 50051);
      expect(loaded.protocol, Protocol.grpc);
      expect(loaded.earThreshold, 0.2);
      expect(loaded.profileName, 'Default');
      expect(loaded.profiles, ['Default']);
    });

    test('load with invalid protocol falls back to grpc', () async {
      SharedPreferences.setMockInitialValues({
        'settings_protocol': 'invalidProtocol',
      });
      final prefs = await SharedPreferences.getInstance();

      final loaded = SettingsState.load(prefs);
      expect(loaded.protocol, Protocol.grpc);
    });
  });
}
