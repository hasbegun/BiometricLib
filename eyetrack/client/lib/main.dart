import 'package:flutter/material.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:eyetrack/core/gaze_calibration.dart';
import 'package:eyetrack/l10n/app_localizations.dart';
import 'package:eyetrack/presentation/screens/calibration_screen.dart';
import 'package:eyetrack/presentation/screens/settings_screen.dart';
import 'package:eyetrack/presentation/screens/tracking_screen.dart';

void main() {
  runApp(const ProviderScope(child: EyetrackApp()));
}

class EyetrackApp extends ConsumerWidget {
  const EyetrackApp({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    return MaterialApp(
      title: 'EyeTrack',
      localizationsDelegates: const [
        AppLocalizations.delegate,
        GlobalMaterialLocalizations.delegate,
        GlobalWidgetsLocalizations.delegate,
        GlobalCupertinoLocalizations.delegate,
      ],
      supportedLocales: AppLocalizations.supportedLocales,
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.indigo),
        useMaterial3: true,
      ),
      darkTheme: ThemeData(
        colorScheme: ColorScheme.fromSeed(
          seedColor: Colors.indigo,
          brightness: Brightness.dark,
        ),
        useMaterial3: true,
      ),
      home: const HomeScreen(),
    );
  }
}

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  int _currentIndex = 0;
  final _trackingKey = GlobalKey<TrackingScreenState>();
  bool _isCalibrated = false;
  GazeCalibrationData? _calibrationData;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);

    return Scaffold(
      body: IndexedStack(
        index: _currentIndex,
        children: [
          // Tab 0: Tracking
          TrackingScreen(
            key: _trackingKey,
            calibrationData: _calibrationData,
          ),
          // Tab 1: Settings
          SettingsScreen(
            onSave: (settings) {
              ScaffoldMessenger.of(context).showSnackBar(
                SnackBar(content: Text(l10n.save)),
              );
            },
          ),
        ],
      ),
      bottomNavigationBar: NavigationBar(
        selectedIndex: _currentIndex,
        onDestinationSelected: (index) {
          setState(() => _currentIndex = index);
        },
        destinations: [
          NavigationDestination(
            icon: const Icon(Icons.visibility_outlined),
            selectedIcon: const Icon(Icons.visibility),
            label: l10n.homeTitle,
          ),
          NavigationDestination(
            icon: const Icon(Icons.settings_outlined),
            selectedIcon: const Icon(Icons.settings),
            label: l10n.settings,
          ),
        ],
      ),
    );
  }

  void _showCalibrationPrompt(BuildContext context, AppLocalizations l10n) {
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        icon: const Icon(Icons.grid_on, size: 48),
        title: Text(l10n.calibrationTitle),
        content: Text(l10n.calibrationInstructions),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: Text(l10n.cancel),
          ),
          FilledButton(
            onPressed: () {
              Navigator.pop(ctx);
              _openCalibration(context);
            },
            child: Text(l10n.calibrate),
          ),
        ],
      ),
    );
  }

  void _openCalibration(BuildContext context) {
    // Get the camera controller from tracking screen if available
    final trackingState = _trackingKey.currentState;
    final macController = trackingState?.macCameraController;

    Navigator.of(context).push(
      MaterialPageRoute(
        builder: (_) => CalibrationScreen(
          macController: macController,
          onCalibrationComplete: (points, calData) {
            setState(() {
              _isCalibrated = true;
              _calibrationData = calData;
            });

            Navigator.of(context).pop();

            // Apply calibration to tracking screen
            if (calData != null) {
              _trackingKey.currentState?.setCalibration(calData);
            }

            ScaffoldMessenger.of(context).showSnackBar(
              SnackBar(
                content: Text(AppLocalizations.of(context).calibrationComplete),
                backgroundColor: Colors.green,
              ),
            );
          },
          onCancel: () => Navigator.of(context).pop(),
        ),
      ),
    );
  }
}
