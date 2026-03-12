import 'package:flutter/foundation.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'package:eyetrack/core/platform_config.dart';

@immutable
class SettingsState {
  const SettingsState({
    this.serverUrl = 'localhost',
    this.port = 50051,
    this.protocol = Protocol.grpc,
    this.earThreshold = 0.2,
    this.profileName = 'Default',
    this.profiles = const ['Default'],
  });

  final String serverUrl;
  final int port;
  final Protocol protocol;
  final double earThreshold;
  final String profileName;
  final List<String> profiles;

  static const _keyServerUrl = 'settings_server_url';
  static const _keyPort = 'settings_port';
  static const _keyProtocol = 'settings_protocol';
  static const _keyEarThreshold = 'settings_ear_threshold';
  static const _keyProfileName = 'settings_profile_name';
  static const _keyProfiles = 'settings_profiles';

  int get defaultPortForProtocol {
    switch (protocol) {
      case Protocol.grpc:
        return 50051;
      case Protocol.webSocket:
        return 8080;
      case Protocol.mqtt:
        return 1883;
    }
  }

  SettingsState copyWith({
    String? serverUrl,
    int? port,
    Protocol? protocol,
    double? earThreshold,
    String? profileName,
    List<String>? profiles,
  }) {
    return SettingsState(
      serverUrl: serverUrl ?? this.serverUrl,
      port: port ?? this.port,
      protocol: protocol ?? this.protocol,
      earThreshold: earThreshold ?? this.earThreshold,
      profileName: profileName ?? this.profileName,
      profiles: profiles ?? this.profiles,
    );
  }

  Future<void> save(SharedPreferences prefs) async {
    await prefs.setString(_keyServerUrl, serverUrl);
    await prefs.setInt(_keyPort, port);
    await prefs.setString(_keyProtocol, protocol.name);
    await prefs.setDouble(_keyEarThreshold, earThreshold);
    await prefs.setString(_keyProfileName, profileName);
    await prefs.setStringList(_keyProfiles, profiles);
  }

  static SettingsState load(SharedPreferences prefs) {
    final protocolName = prefs.getString(_keyProtocol);
    final protocol = Protocol.values.where((p) => p.name == protocolName).firstOrNull;

    return SettingsState(
      serverUrl: prefs.getString(_keyServerUrl) ?? 'localhost',
      port: prefs.getInt(_keyPort) ?? 50051,
      protocol: protocol ?? Protocol.grpc,
      earThreshold: prefs.getDouble(_keyEarThreshold) ?? 0.2,
      profileName: prefs.getString(_keyProfileName) ?? 'Default',
      profiles: prefs.getStringList(_keyProfiles) ?? const ['Default'],
    );
  }
}
