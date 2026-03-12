import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'package:eyetrack/core/platform_config.dart';
import 'package:eyetrack/core/settings_state.dart';
import 'package:eyetrack/l10n/app_localizations.dart';

class SettingsScreen extends StatefulWidget {
  const SettingsScreen({
    super.key,
    this.initialSettings,
    this.onSave,
    this.prefs,
  });

  final SettingsState? initialSettings;
  final ValueChanged<SettingsState>? onSave;
  final SharedPreferences? prefs;

  @override
  State<SettingsScreen> createState() => SettingsScreenState();
}

@visibleForTesting
class SettingsScreenState extends State<SettingsScreen> {
  late SettingsState _state;
  late TextEditingController _serverUrlController;
  late TextEditingController _portController;

  SettingsState get state => _state;

  @override
  void initState() {
    super.initState();
    _state = widget.initialSettings ?? const SettingsState();
    _serverUrlController = TextEditingController(text: _state.serverUrl);
    _portController = TextEditingController(text: _state.port.toString());
  }

  @override
  void dispose() {
    _serverUrlController.dispose();
    _portController.dispose();
    super.dispose();
  }

  void updateServerUrl(String url) {
    setState(() {
      _state = _state.copyWith(serverUrl: url);
    });
  }

  void updatePort(String portStr) {
    final port = int.tryParse(portStr);
    if (port != null && port > 0 && port <= 65535) {
      setState(() {
        _state = _state.copyWith(port: port);
      });
    }
  }

  void updateProtocol(Protocol protocol) {
    setState(() {
      _state = _state.copyWith(
        protocol: protocol,
        port: _state.copyWith(protocol: protocol).defaultPortForProtocol,
      );
      _portController.text = _state.port.toString();
    });
  }

  void updateEarThreshold(double value) {
    setState(() {
      _state = _state.copyWith(earThreshold: value);
    });
  }

  void selectProfile(String name) {
    setState(() {
      _state = _state.copyWith(profileName: name);
    });
  }

  void addProfile(String name) {
    if (name.isEmpty || _state.profiles.contains(name)) return;
    setState(() {
      _state = _state.copyWith(
        profiles: [..._state.profiles, name],
        profileName: name,
      );
    });
  }

  void deleteProfile(String name) {
    if (_state.profiles.length <= 1) return;
    final updated = _state.profiles.where((p) => p != name).toList();
    setState(() {
      _state = _state.copyWith(
        profiles: updated,
        profileName: _state.profileName == name ? updated.first : null,
      );
    });
  }

  Future<void> save() async {
    if (widget.prefs != null) {
      await _state.save(widget.prefs!);
    }
    widget.onSave?.call(_state);
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);

    return Scaffold(
      appBar: AppBar(
        title: Text(l10n.settings),
        actions: [
          TextButton(
            onPressed: save,
            child: Text(l10n.save),
          ),
        ],
      ),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          // Server URL
          TextField(
            controller: _serverUrlController,
            decoration: InputDecoration(
              labelText: l10n.serverUrl,
              border: const OutlineInputBorder(),
            ),
            onChanged: updateServerUrl,
          ),
          const SizedBox(height: 16),

          // Port
          TextField(
            controller: _portController,
            decoration: InputDecoration(
              labelText: l10n.port,
              border: const OutlineInputBorder(),
            ),
            keyboardType: TextInputType.number,
            onChanged: updatePort,
          ),
          const SizedBox(height: 16),

          // Protocol
          Text(l10n.protocol, style: Theme.of(context).textTheme.titleSmall),
          const SizedBox(height: 8),
          SegmentedButton<Protocol>(
            segments: [
              ButtonSegment(value: Protocol.grpc, label: Text(l10n.protocolGrpc)),
              ButtonSegment(value: Protocol.webSocket, label: Text(l10n.protocolWebSocket)),
              ButtonSegment(value: Protocol.mqtt, label: Text(l10n.protocolMqtt)),
            ],
            selected: {_state.protocol},
            onSelectionChanged: (selected) => updateProtocol(selected.first),
          ),
          const SizedBox(height: 24),

          // EAR Threshold
          Text(l10n.earThreshold, style: Theme.of(context).textTheme.titleSmall),
          Slider(
            value: _state.earThreshold,
            min: 0.1,
            max: 0.4,
            divisions: 30,
            label: _state.earThreshold.toStringAsFixed(2),
            onChanged: updateEarThreshold,
          ),
          const SizedBox(height: 24),

          // Profile
          Text(l10n.profile, style: Theme.of(context).textTheme.titleSmall),
          const SizedBox(height: 8),
          Wrap(
            spacing: 8,
            children: _state.profiles.map((name) {
              final isSelected = name == _state.profileName;
              return ChoiceChip(
                label: Text(name),
                selected: isSelected,
                onSelected: (_) => selectProfile(name),
              );
            }).toList(),
          ),
          const SizedBox(height: 8),
          Row(
            children: [
              TextButton.icon(
                onPressed: () => _showAddProfileDialog(context, l10n),
                icon: const Icon(Icons.add),
                label: Text(l10n.addProfile),
              ),
              if (_state.profiles.length > 1)
                TextButton.icon(
                  onPressed: () => deleteProfile(_state.profileName),
                  icon: const Icon(Icons.delete),
                  label: Text(l10n.deleteProfile),
                ),
            ],
          ),
        ],
      ),
    );
  }

  void _showAddProfileDialog(BuildContext context, AppLocalizations l10n) {
    final controller = TextEditingController();
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: Text(l10n.addProfile),
        content: TextField(
          controller: controller,
          decoration: InputDecoration(labelText: l10n.profileName),
          autofocus: true,
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: Text(l10n.cancel),
          ),
          TextButton(
            onPressed: () {
              addProfile(controller.text.trim());
              Navigator.pop(ctx);
            },
            child: Text(l10n.add),
          ),
        ],
      ),
    );
  }
}
