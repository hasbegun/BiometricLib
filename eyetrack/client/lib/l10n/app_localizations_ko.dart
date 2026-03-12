// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for Korean (`ko`).
class AppLocalizationsKo extends AppLocalizations {
  AppLocalizationsKo([String locale = 'ko']) : super(locale);

  @override
  String get appTitle => '아이트랙';

  @override
  String get homeTitle => '시선 추적';

  @override
  String get calibrate => '보정';

  @override
  String get startTracking => '추적 시작';

  @override
  String get stopTracking => '추적 중지';

  @override
  String get settings => '설정';

  @override
  String get serverUrl => '서버 URL';

  @override
  String get protocol => '프로토콜';

  @override
  String get protocolGrpc => 'gRPC';

  @override
  String get protocolWebSocket => 'WebSocket';

  @override
  String get protocolMqtt => 'MQTT';

  @override
  String get connected => '연결됨';

  @override
  String get disconnected => '연결 안됨';

  @override
  String get connecting => '연결 중...';

  @override
  String get calibrationTitle => '보정';

  @override
  String get calibrationInstructions => '화면에 나타나는 각 점을 바라보세요.';

  @override
  String get calibrationComplete => '보정이 완료되었습니다!';

  @override
  String get calibrationFailed => '보정에 실패했습니다. 다시 시도하세요.';

  @override
  String accuracy(String value) {
    return '정확도: $value%';
  }

  @override
  String fps(String value) {
    return 'FPS: $value';
  }

  @override
  String latency(String value) {
    return '지연: ${value}ms';
  }

  @override
  String get blinkSingle => '단일 깜빡임';

  @override
  String get blinkDouble => '이중 깜빡임';

  @override
  String get blinkLong => '긴 깜빡임';

  @override
  String get debugMode => '디버그 모드';

  @override
  String get earThreshold => 'EAR 임계값';

  @override
  String get cameraPermissionRequired => '시선 추적을 위해 카메라 권한이 필요합니다.';

  @override
  String get noCameraAvailable => '이 기기에서 사용할 수 있는 카메라가 없습니다.';

  @override
  String get language => '언어';

  @override
  String get english => '영어';

  @override
  String get korean => '한국어';

  @override
  String get cancel => '취소';

  @override
  String get save => '저장';

  @override
  String get error => '오류';

  @override
  String get retry => '재시도';

  @override
  String get connectionError => '서버에 연결할 수 없습니다.';

  @override
  String get port => '포트';

  @override
  String get profile => '프로필';

  @override
  String get addProfile => '프로필 추가';

  @override
  String get deleteProfile => '프로필 삭제';

  @override
  String get profileName => '프로필 이름';

  @override
  String get add => '추가';

  @override
  String get delete => '삭제';

  @override
  String gazeTile(int number) {
    return '타일: $number';
  }

  @override
  String get noGaze => '시선 미감지';
}
