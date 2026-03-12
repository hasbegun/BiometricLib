//
//  Generated code. Do not modify.
//  source: eyetrack.proto
//
// @dart = 2.12

// ignore_for_file: annotate_overrides, camel_case_types, comment_references
// ignore_for_file: constant_identifier_names, library_prefixes
// ignore_for_file: non_constant_identifier_names, prefer_final_fields
// ignore_for_file: unnecessary_import, unnecessary_this, unused_import

import 'dart:core' as $core;

import 'package:protobuf/protobuf.dart' as $pb;

class BlinkType extends $pb.ProtobufEnum {
  static const BlinkType BLINK_NONE = BlinkType._(0, _omitEnumNames ? '' : 'BLINK_NONE');
  static const BlinkType BLINK_SINGLE = BlinkType._(1, _omitEnumNames ? '' : 'BLINK_SINGLE');
  static const BlinkType BLINK_DOUBLE = BlinkType._(2, _omitEnumNames ? '' : 'BLINK_DOUBLE');
  static const BlinkType BLINK_LONG = BlinkType._(3, _omitEnumNames ? '' : 'BLINK_LONG');

  static const $core.List<BlinkType> values = <BlinkType> [
    BLINK_NONE,
    BLINK_SINGLE,
    BLINK_DOUBLE,
    BLINK_LONG,
  ];

  static final $core.Map<$core.int, BlinkType> _byValue = $pb.ProtobufEnum.initByValue(values);
  static BlinkType? valueOf($core.int value) => _byValue[value];

  const BlinkType._($core.int v, $core.String n) : super(v, n);
}


const _omitEnumNames = $core.bool.fromEnvironment('protobuf.omit_enum_names');
