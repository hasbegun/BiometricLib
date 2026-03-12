//
//  Generated code. Do not modify.
//  source: eyetrack.proto
//
// @dart = 2.12

// ignore_for_file: annotate_overrides, camel_case_types, comment_references
// ignore_for_file: constant_identifier_names, library_prefixes
// ignore_for_file: non_constant_identifier_names, prefer_final_fields
// ignore_for_file: unnecessary_import, unnecessary_this, unused_import

import 'dart:convert' as $convert;
import 'dart:core' as $core;
import 'dart:typed_data' as $typed_data;

@$core.Deprecated('Use blinkTypeDescriptor instead')
const BlinkType$json = {
  '1': 'BlinkType',
  '2': [
    {'1': 'BLINK_NONE', '2': 0},
    {'1': 'BLINK_SINGLE', '2': 1},
    {'1': 'BLINK_DOUBLE', '2': 2},
    {'1': 'BLINK_LONG', '2': 3},
  ],
};

/// Descriptor for `BlinkType`. Decode as a `google.protobuf.EnumDescriptorProto`.
final $typed_data.Uint8List blinkTypeDescriptor = $convert.base64Decode(
    'CglCbGlua1R5cGUSDgoKQkxJTktfTk9ORRAAEhAKDEJMSU5LX1NJTkdMRRABEhAKDEJMSU5LX0'
    'RPVUJMRRACEg4KCkJMSU5LX0xPTkcQAw==');

@$core.Deprecated('Use point2DDescriptor instead')
const Point2D$json = {
  '1': 'Point2D',
  '2': [
    {'1': 'x', '3': 1, '4': 1, '5': 2, '10': 'x'},
    {'1': 'y', '3': 2, '4': 1, '5': 2, '10': 'y'},
  ],
};

/// Descriptor for `Point2D`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List point2DDescriptor = $convert.base64Decode(
    'CgdQb2ludDJEEgwKAXgYASABKAJSAXgSDAoBeRgCIAEoAlIBeQ==');

@$core.Deprecated('Use eyeLandmarksDescriptor instead')
const EyeLandmarks$json = {
  '1': 'EyeLandmarks',
  '2': [
    {'1': 'left', '3': 1, '4': 3, '5': 11, '6': '.eyetrack.proto.Point2D', '10': 'left'},
    {'1': 'right', '3': 2, '4': 3, '5': 11, '6': '.eyetrack.proto.Point2D', '10': 'right'},
  ],
};

/// Descriptor for `EyeLandmarks`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List eyeLandmarksDescriptor = $convert.base64Decode(
    'CgxFeWVMYW5kbWFya3MSKwoEbGVmdBgBIAMoCzIXLmV5ZXRyYWNrLnByb3RvLlBvaW50MkRSBG'
    'xlZnQSLQoFcmlnaHQYAiADKAsyFy5leWV0cmFjay5wcm90by5Qb2ludDJEUgVyaWdodA==');

@$core.Deprecated('Use pupilInfoDescriptor instead')
const PupilInfo$json = {
  '1': 'PupilInfo',
  '2': [
    {'1': 'center', '3': 1, '4': 1, '5': 11, '6': '.eyetrack.proto.Point2D', '10': 'center'},
    {'1': 'radius', '3': 2, '4': 1, '5': 2, '10': 'radius'},
    {'1': 'confidence', '3': 3, '4': 1, '5': 2, '10': 'confidence'},
  ],
};

/// Descriptor for `PupilInfo`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List pupilInfoDescriptor = $convert.base64Decode(
    'CglQdXBpbEluZm8SLwoGY2VudGVyGAEgASgLMhcuZXlldHJhY2sucHJvdG8uUG9pbnQyRFIGY2'
    'VudGVyEhYKBnJhZGl1cxgCIAEoAlIGcmFkaXVzEh4KCmNvbmZpZGVuY2UYAyABKAJSCmNvbmZp'
    'ZGVuY2U=');

@$core.Deprecated('Use rect2fDescriptor instead')
const Rect2f$json = {
  '1': 'Rect2f',
  '2': [
    {'1': 'x', '3': 1, '4': 1, '5': 2, '10': 'x'},
    {'1': 'y', '3': 2, '4': 1, '5': 2, '10': 'y'},
    {'1': 'width', '3': 3, '4': 1, '5': 2, '10': 'width'},
    {'1': 'height', '3': 4, '4': 1, '5': 2, '10': 'height'},
  ],
};

/// Descriptor for `Rect2f`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List rect2fDescriptor = $convert.base64Decode(
    'CgZSZWN0MmYSDAoBeBgBIAEoAlIBeBIMCgF5GAIgASgCUgF5EhQKBXdpZHRoGAMgASgCUgV3aW'
    'R0aBIWCgZoZWlnaHQYBCABKAJSBmhlaWdodA==');

@$core.Deprecated('Use faceROIDescriptor instead')
const FaceROI$json = {
  '1': 'FaceROI',
  '2': [
    {'1': 'bounding_box', '3': 1, '4': 1, '5': 11, '6': '.eyetrack.proto.Rect2f', '10': 'boundingBox'},
    {'1': 'landmarks', '3': 2, '4': 3, '5': 11, '6': '.eyetrack.proto.Point2D', '10': 'landmarks'},
    {'1': 'confidence', '3': 3, '4': 1, '5': 2, '10': 'confidence'},
  ],
};

/// Descriptor for `FaceROI`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List faceROIDescriptor = $convert.base64Decode(
    'CgdGYWNlUk9JEjkKDGJvdW5kaW5nX2JveBgBIAEoCzIWLmV5ZXRyYWNrLnByb3RvLlJlY3QyZl'
    'ILYm91bmRpbmdCb3gSNQoJbGFuZG1hcmtzGAIgAygLMhcuZXlldHJhY2sucHJvdG8uUG9pbnQy'
    'RFIJbGFuZG1hcmtzEh4KCmNvbmZpZGVuY2UYAyABKAJSCmNvbmZpZGVuY2U=');

@$core.Deprecated('Use frameDataDescriptor instead')
const FrameData$json = {
  '1': 'FrameData',
  '2': [
    {'1': 'frame', '3': 1, '4': 1, '5': 12, '10': 'frame'},
    {'1': 'frame_id', '3': 2, '4': 1, '5': 4, '10': 'frameId'},
    {'1': 'timestamp_ns', '3': 3, '4': 1, '5': 3, '10': 'timestampNs'},
    {'1': 'client_id', '3': 4, '4': 1, '5': 9, '10': 'clientId'},
    {'1': 'width', '3': 5, '4': 1, '5': 13, '10': 'width'},
    {'1': 'height', '3': 6, '4': 1, '5': 13, '10': 'height'},
    {'1': 'channels', '3': 7, '4': 1, '5': 13, '10': 'channels'},
  ],
};

/// Descriptor for `FrameData`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List frameDataDescriptor = $convert.base64Decode(
    'CglGcmFtZURhdGESFAoFZnJhbWUYASABKAxSBWZyYW1lEhkKCGZyYW1lX2lkGAIgASgEUgdmcm'
    'FtZUlkEiEKDHRpbWVzdGFtcF9ucxgDIAEoA1ILdGltZXN0YW1wTnMSGwoJY2xpZW50X2lkGAQg'
    'ASgJUghjbGllbnRJZBIUCgV3aWR0aBgFIAEoDVIFd2lkdGgSFgoGaGVpZ2h0GAYgASgNUgZoZW'
    'lnaHQSGgoIY2hhbm5lbHMYByABKA1SCGNoYW5uZWxz');

@$core.Deprecated('Use featureDataDescriptor instead')
const FeatureData$json = {
  '1': 'FeatureData',
  '2': [
    {'1': 'eye_landmarks', '3': 1, '4': 1, '5': 11, '6': '.eyetrack.proto.EyeLandmarks', '10': 'eyeLandmarks'},
    {'1': 'left_pupil', '3': 2, '4': 1, '5': 11, '6': '.eyetrack.proto.PupilInfo', '10': 'leftPupil'},
    {'1': 'right_pupil', '3': 3, '4': 1, '5': 11, '6': '.eyetrack.proto.PupilInfo', '10': 'rightPupil'},
    {'1': 'ear_left', '3': 4, '4': 1, '5': 2, '10': 'earLeft'},
    {'1': 'ear_right', '3': 5, '4': 1, '5': 2, '10': 'earRight'},
    {'1': 'face_roi', '3': 6, '4': 1, '5': 11, '6': '.eyetrack.proto.FaceROI', '10': 'faceRoi'},
    {'1': 'frame_id', '3': 7, '4': 1, '5': 4, '10': 'frameId'},
  ],
};

/// Descriptor for `FeatureData`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List featureDataDescriptor = $convert.base64Decode(
    'CgtGZWF0dXJlRGF0YRJBCg1leWVfbGFuZG1hcmtzGAEgASgLMhwuZXlldHJhY2sucHJvdG8uRX'
    'llTGFuZG1hcmtzUgxleWVMYW5kbWFya3MSOAoKbGVmdF9wdXBpbBgCIAEoCzIZLmV5ZXRyYWNr'
    'LnByb3RvLlB1cGlsSW5mb1IJbGVmdFB1cGlsEjoKC3JpZ2h0X3B1cGlsGAMgASgLMhkuZXlldH'
    'JhY2sucHJvdG8uUHVwaWxJbmZvUgpyaWdodFB1cGlsEhkKCGVhcl9sZWZ0GAQgASgCUgdlYXJM'
    'ZWZ0EhsKCWVhcl9yaWdodBgFIAEoAlIIZWFyUmlnaHQSMgoIZmFjZV9yb2kYBiABKAsyFy5leW'
    'V0cmFjay5wcm90by5GYWNlUk9JUgdmYWNlUm9pEhkKCGZyYW1lX2lkGAcgASgEUgdmcmFtZUlk');

@$core.Deprecated('Use gazeEventDescriptor instead')
const GazeEvent$json = {
  '1': 'GazeEvent',
  '2': [
    {'1': 'gaze_point', '3': 1, '4': 1, '5': 11, '6': '.eyetrack.proto.Point2D', '10': 'gazePoint'},
    {'1': 'confidence', '3': 2, '4': 1, '5': 2, '10': 'confidence'},
    {'1': 'blink', '3': 3, '4': 1, '5': 14, '6': '.eyetrack.proto.BlinkType', '10': 'blink'},
    {'1': 'timestamp_ns', '3': 4, '4': 1, '5': 3, '10': 'timestampNs'},
    {'1': 'client_id', '3': 5, '4': 1, '5': 9, '10': 'clientId'},
    {'1': 'frame_id', '3': 6, '4': 1, '5': 4, '10': 'frameId'},
  ],
};

/// Descriptor for `GazeEvent`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List gazeEventDescriptor = $convert.base64Decode(
    'CglHYXplRXZlbnQSNgoKZ2F6ZV9wb2ludBgBIAEoCzIXLmV5ZXRyYWNrLnByb3RvLlBvaW50Mk'
    'RSCWdhemVQb2ludBIeCgpjb25maWRlbmNlGAIgASgCUgpjb25maWRlbmNlEi8KBWJsaW5rGAMg'
    'ASgOMhkuZXlldHJhY2sucHJvdG8uQmxpbmtUeXBlUgVibGluaxIhCgx0aW1lc3RhbXBfbnMYBC'
    'ABKANSC3RpbWVzdGFtcE5zEhsKCWNsaWVudF9pZBgFIAEoCVIIY2xpZW50SWQSGQoIZnJhbWVf'
    'aWQYBiABKARSB2ZyYW1lSWQ=');

@$core.Deprecated('Use calibrationPointDescriptor instead')
const CalibrationPoint$json = {
  '1': 'CalibrationPoint',
  '2': [
    {'1': 'screen_point', '3': 1, '4': 1, '5': 11, '6': '.eyetrack.proto.Point2D', '10': 'screenPoint'},
    {'1': 'pupil_observation', '3': 2, '4': 1, '5': 11, '6': '.eyetrack.proto.PupilInfo', '10': 'pupilObservation'},
  ],
};

/// Descriptor for `CalibrationPoint`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List calibrationPointDescriptor = $convert.base64Decode(
    'ChBDYWxpYnJhdGlvblBvaW50EjoKDHNjcmVlbl9wb2ludBgBIAEoCzIXLmV5ZXRyYWNrLnByb3'
    'RvLlBvaW50MkRSC3NjcmVlblBvaW50EkYKEXB1cGlsX29ic2VydmF0aW9uGAIgASgLMhkuZXll'
    'dHJhY2sucHJvdG8uUHVwaWxJbmZvUhBwdXBpbE9ic2VydmF0aW9u');

@$core.Deprecated('Use calibrationRequestDescriptor instead')
const CalibrationRequest$json = {
  '1': 'CalibrationRequest',
  '2': [
    {'1': 'user_id', '3': 1, '4': 1, '5': 9, '10': 'userId'},
    {'1': 'points', '3': 2, '4': 3, '5': 11, '6': '.eyetrack.proto.CalibrationPoint', '10': 'points'},
  ],
};

/// Descriptor for `CalibrationRequest`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List calibrationRequestDescriptor = $convert.base64Decode(
    'ChJDYWxpYnJhdGlvblJlcXVlc3QSFwoHdXNlcl9pZBgBIAEoCVIGdXNlcklkEjgKBnBvaW50cx'
    'gCIAMoCzIgLmV5ZXRyYWNrLnByb3RvLkNhbGlicmF0aW9uUG9pbnRSBnBvaW50cw==');

@$core.Deprecated('Use calibrationResponseDescriptor instead')
const CalibrationResponse$json = {
  '1': 'CalibrationResponse',
  '2': [
    {'1': 'success', '3': 1, '4': 1, '5': 8, '10': 'success'},
    {'1': 'error_message', '3': 2, '4': 1, '5': 9, '10': 'errorMessage'},
    {'1': 'profile_id', '3': 3, '4': 1, '5': 9, '10': 'profileId'},
  ],
};

/// Descriptor for `CalibrationResponse`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List calibrationResponseDescriptor = $convert.base64Decode(
    'ChNDYWxpYnJhdGlvblJlc3BvbnNlEhgKB3N1Y2Nlc3MYASABKAhSB3N1Y2Nlc3MSIwoNZXJyb3'
    'JfbWVzc2FnZRgCIAEoCVIMZXJyb3JNZXNzYWdlEh0KCnByb2ZpbGVfaWQYAyABKAlSCXByb2Zp'
    'bGVJZA==');

@$core.Deprecated('Use streamGazeRequestDescriptor instead')
const StreamGazeRequest$json = {
  '1': 'StreamGazeRequest',
  '2': [
    {'1': 'client_id', '3': 1, '4': 1, '5': 9, '10': 'clientId'},
  ],
};

/// Descriptor for `StreamGazeRequest`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List streamGazeRequestDescriptor = $convert.base64Decode(
    'ChFTdHJlYW1HYXplUmVxdWVzdBIbCgljbGllbnRfaWQYASABKAlSCGNsaWVudElk');

@$core.Deprecated('Use statusRequestDescriptor instead')
const StatusRequest$json = {
  '1': 'StatusRequest',
};

/// Descriptor for `StatusRequest`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List statusRequestDescriptor = $convert.base64Decode(
    'Cg1TdGF0dXNSZXF1ZXN0');

@$core.Deprecated('Use statusResponseDescriptor instead')
const StatusResponse$json = {
  '1': 'StatusResponse',
  '2': [
    {'1': 'is_running', '3': 1, '4': 1, '5': 8, '10': 'isRunning'},
    {'1': 'connected_clients', '3': 2, '4': 1, '5': 13, '10': 'connectedClients'},
    {'1': 'fps', '3': 3, '4': 1, '5': 2, '10': 'fps'},
    {'1': 'version', '3': 4, '4': 1, '5': 9, '10': 'version'},
  ],
};

/// Descriptor for `StatusResponse`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List statusResponseDescriptor = $convert.base64Decode(
    'Cg5TdGF0dXNSZXNwb25zZRIdCgppc19ydW5uaW5nGAEgASgIUglpc1J1bm5pbmcSKwoRY29ubm'
    'VjdGVkX2NsaWVudHMYAiABKA1SEGNvbm5lY3RlZENsaWVudHMSEAoDZnBzGAMgASgCUgNmcHMS'
    'GAoHdmVyc2lvbhgEIAEoCVIHdmVyc2lvbg==');

