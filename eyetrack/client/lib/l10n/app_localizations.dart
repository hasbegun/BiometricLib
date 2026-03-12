import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:intl/intl.dart' as intl;

import 'app_localizations_en.dart';
import 'app_localizations_ko.dart';

// ignore_for_file: type=lint

/// Callers can lookup localized strings with an instance of AppLocalizations
/// returned by `AppLocalizations.of(context)`.
///
/// Applications need to include `AppLocalizations.delegate()` in their app's
/// `localizationDelegates` list, and the locales they support in the app's
/// `supportedLocales` list. For example:
///
/// ```dart
/// import 'l10n/app_localizations.dart';
///
/// return MaterialApp(
///   localizationsDelegates: AppLocalizations.localizationsDelegates,
///   supportedLocales: AppLocalizations.supportedLocales,
///   home: MyApplicationHome(),
/// );
/// ```
///
/// ## Update pubspec.yaml
///
/// Please make sure to update your pubspec.yaml to include the following
/// packages:
///
/// ```yaml
/// dependencies:
///   # Internationalization support.
///   flutter_localizations:
///     sdk: flutter
///   intl: any # Use the pinned version from flutter_localizations
///
///   # Rest of dependencies
/// ```
///
/// ## iOS Applications
///
/// iOS applications define key application metadata, including supported
/// locales, in an Info.plist file that is built into the application bundle.
/// To configure the locales supported by your app, you’ll need to edit this
/// file.
///
/// First, open your project’s ios/Runner.xcworkspace Xcode workspace file.
/// Then, in the Project Navigator, open the Info.plist file under the Runner
/// project’s Runner folder.
///
/// Next, select the Information Property List item, select Add Item from the
/// Editor menu, then select Localizations from the pop-up menu.
///
/// Select and expand the newly-created Localizations item then, for each
/// locale your application supports, add a new item and select the locale
/// you wish to add from the pop-up menu in the Value field. This list should
/// be consistent with the languages listed in the AppLocalizations.supportedLocales
/// property.
abstract class AppLocalizations {
  AppLocalizations(String locale)
    : localeName = intl.Intl.canonicalizedLocale(locale.toString());

  final String localeName;

  static AppLocalizations of(BuildContext context) {
    return Localizations.of<AppLocalizations>(context, AppLocalizations)!;
  }

  static const LocalizationsDelegate<AppLocalizations> delegate =
      _AppLocalizationsDelegate();

  /// A list of this localizations delegate along with the default localizations
  /// delegates.
  ///
  /// Returns a list of localizations delegates containing this delegate along with
  /// GlobalMaterialLocalizations.delegate, GlobalCupertinoLocalizations.delegate,
  /// and GlobalWidgetsLocalizations.delegate.
  ///
  /// Additional delegates can be added by appending to this list in
  /// MaterialApp. This list does not have to be used at all if a custom list
  /// of delegates is preferred or required.
  static const List<LocalizationsDelegate<dynamic>> localizationsDelegates =
      <LocalizationsDelegate<dynamic>>[
        delegate,
        GlobalMaterialLocalizations.delegate,
        GlobalCupertinoLocalizations.delegate,
        GlobalWidgetsLocalizations.delegate,
      ];

  /// A list of this localizations delegate's supported locales.
  static const List<Locale> supportedLocales = <Locale>[
    Locale('en'),
    Locale('ko'),
  ];

  /// Application title
  ///
  /// In en, this message translates to:
  /// **'EyeTrack'**
  String get appTitle;

  /// Home screen title
  ///
  /// In en, this message translates to:
  /// **'Eye Tracking'**
  String get homeTitle;

  /// Calibrate button label
  ///
  /// In en, this message translates to:
  /// **'Calibrate'**
  String get calibrate;

  /// Start tracking button
  ///
  /// In en, this message translates to:
  /// **'Start Tracking'**
  String get startTracking;

  /// Stop tracking button
  ///
  /// In en, this message translates to:
  /// **'Stop Tracking'**
  String get stopTracking;

  /// Settings screen title
  ///
  /// In en, this message translates to:
  /// **'Settings'**
  String get settings;

  /// Server URL setting label
  ///
  /// In en, this message translates to:
  /// **'Server URL'**
  String get serverUrl;

  /// Protocol selection label
  ///
  /// In en, this message translates to:
  /// **'Protocol'**
  String get protocol;

  /// gRPC protocol option
  ///
  /// In en, this message translates to:
  /// **'gRPC'**
  String get protocolGrpc;

  /// WebSocket protocol option
  ///
  /// In en, this message translates to:
  /// **'WebSocket'**
  String get protocolWebSocket;

  /// MQTT protocol option
  ///
  /// In en, this message translates to:
  /// **'MQTT'**
  String get protocolMqtt;

  /// Connection status: connected
  ///
  /// In en, this message translates to:
  /// **'Connected'**
  String get connected;

  /// Connection status: disconnected
  ///
  /// In en, this message translates to:
  /// **'Disconnected'**
  String get disconnected;

  /// Connection status: connecting
  ///
  /// In en, this message translates to:
  /// **'Connecting...'**
  String get connecting;

  /// Calibration screen title
  ///
  /// In en, this message translates to:
  /// **'Calibration'**
  String get calibrationTitle;

  /// Calibration instructions
  ///
  /// In en, this message translates to:
  /// **'Look at each point as it appears on the screen.'**
  String get calibrationInstructions;

  /// Calibration success message
  ///
  /// In en, this message translates to:
  /// **'Calibration complete!'**
  String get calibrationComplete;

  /// Calibration failure message
  ///
  /// In en, this message translates to:
  /// **'Calibration failed. Please try again.'**
  String get calibrationFailed;

  /// Accuracy display
  ///
  /// In en, this message translates to:
  /// **'Accuracy: {value}%'**
  String accuracy(String value);

  /// Frames per second display
  ///
  /// In en, this message translates to:
  /// **'FPS: {value}'**
  String fps(String value);

  /// Latency display
  ///
  /// In en, this message translates to:
  /// **'Latency: {value}ms'**
  String latency(String value);

  /// Single blink event
  ///
  /// In en, this message translates to:
  /// **'Single Blink'**
  String get blinkSingle;

  /// Double blink event
  ///
  /// In en, this message translates to:
  /// **'Double Blink'**
  String get blinkDouble;

  /// Long blink event
  ///
  /// In en, this message translates to:
  /// **'Long Blink'**
  String get blinkLong;

  /// Debug mode toggle label
  ///
  /// In en, this message translates to:
  /// **'Debug Mode'**
  String get debugMode;

  /// Eye aspect ratio threshold setting
  ///
  /// In en, this message translates to:
  /// **'EAR Threshold'**
  String get earThreshold;

  /// Camera permission message
  ///
  /// In en, this message translates to:
  /// **'Camera permission is required for eye tracking.'**
  String get cameraPermissionRequired;

  /// No camera fallback message
  ///
  /// In en, this message translates to:
  /// **'No camera available on this device.'**
  String get noCameraAvailable;

  /// Language setting label
  ///
  /// In en, this message translates to:
  /// **'Language'**
  String get language;

  /// English language option
  ///
  /// In en, this message translates to:
  /// **'English'**
  String get english;

  /// Korean language option
  ///
  /// In en, this message translates to:
  /// **'Korean'**
  String get korean;

  /// Cancel button
  ///
  /// In en, this message translates to:
  /// **'Cancel'**
  String get cancel;

  /// Save button
  ///
  /// In en, this message translates to:
  /// **'Save'**
  String get save;

  /// Error title
  ///
  /// In en, this message translates to:
  /// **'Error'**
  String get error;

  /// Retry button
  ///
  /// In en, this message translates to:
  /// **'Retry'**
  String get retry;

  /// Connection error message
  ///
  /// In en, this message translates to:
  /// **'Could not connect to the server.'**
  String get connectionError;

  /// Server port setting label
  ///
  /// In en, this message translates to:
  /// **'Port'**
  String get port;

  /// Profile setting label
  ///
  /// In en, this message translates to:
  /// **'Profile'**
  String get profile;

  /// Add profile button
  ///
  /// In en, this message translates to:
  /// **'Add Profile'**
  String get addProfile;

  /// Delete profile button
  ///
  /// In en, this message translates to:
  /// **'Delete Profile'**
  String get deleteProfile;

  /// Profile name input label
  ///
  /// In en, this message translates to:
  /// **'Profile Name'**
  String get profileName;

  /// Add button
  ///
  /// In en, this message translates to:
  /// **'Add'**
  String get add;

  /// Delete button
  ///
  /// In en, this message translates to:
  /// **'Delete'**
  String get delete;

  /// Currently gazed tile number
  ///
  /// In en, this message translates to:
  /// **'Tile: {number}'**
  String gazeTile(int number);

  /// Shown when not tracking or no gaze
  ///
  /// In en, this message translates to:
  /// **'No gaze detected'**
  String get noGaze;
}

class _AppLocalizationsDelegate
    extends LocalizationsDelegate<AppLocalizations> {
  const _AppLocalizationsDelegate();

  @override
  Future<AppLocalizations> load(Locale locale) {
    return SynchronousFuture<AppLocalizations>(lookupAppLocalizations(locale));
  }

  @override
  bool isSupported(Locale locale) =>
      <String>['en', 'ko'].contains(locale.languageCode);

  @override
  bool shouldReload(_AppLocalizationsDelegate old) => false;
}

AppLocalizations lookupAppLocalizations(Locale locale) {
  // Lookup logic when only language code is specified.
  switch (locale.languageCode) {
    case 'en':
      return AppLocalizationsEn();
    case 'ko':
      return AppLocalizationsKo();
  }

  throw FlutterError(
    'AppLocalizations.delegate failed to load unsupported locale "$locale". This is likely '
    'an issue with the localizations generation tool. Please file an issue '
    'on GitHub with a reproducible sample app and the gen-l10n configuration '
    'that was used.',
  );
}
