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

import 'package:fixnum/fixnum.dart' as $fixnum;
import 'package:protobuf/protobuf.dart' as $pb;

import 'eyetrack.pbenum.dart';

export 'eyetrack.pbenum.dart';

class Point2D extends $pb.GeneratedMessage {
  factory Point2D({
    $core.double? x,
    $core.double? y,
  }) {
    final $result = create();
    if (x != null) {
      $result.x = x;
    }
    if (y != null) {
      $result.y = y;
    }
    return $result;
  }
  Point2D._() : super();
  factory Point2D.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory Point2D.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(_omitMessageNames ? '' : 'Point2D', package: const $pb.PackageName(_omitMessageNames ? '' : 'eyetrack.proto'), createEmptyInstance: create)
    ..a<$core.double>(1, _omitFieldNames ? '' : 'x', $pb.PbFieldType.OF)
    ..a<$core.double>(2, _omitFieldNames ? '' : 'y', $pb.PbFieldType.OF)
    ..hasRequiredFields = false
  ;

  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  Point2D clone() => Point2D()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  Point2D copyWith(void Function(Point2D) updates) => super.copyWith((message) => updates(message as Point2D)) as Point2D;

  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  static Point2D create() => Point2D._();
  Point2D createEmptyInstance() => create();
  static $pb.PbList<Point2D> createRepeated() => $pb.PbList<Point2D>();
  @$core.pragma('dart2js:noInline')
  static Point2D getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<Point2D>(create);
  static Point2D? _defaultInstance;

  @$pb.TagNumber(1)
  $core.double get x => $_getN(0);
  @$pb.TagNumber(1)
  set x($core.double v) { $_setFloat(0, v); }
  @$pb.TagNumber(1)
  $core.bool hasX() => $_has(0);
  @$pb.TagNumber(1)
  void clearX() => clearField(1);

  @$pb.TagNumber(2)
  $core.double get y => $_getN(1);
  @$pb.TagNumber(2)
  set y($core.double v) { $_setFloat(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasY() => $_has(1);
  @$pb.TagNumber(2)
  void clearY() => clearField(2);
}

class EyeLandmarks extends $pb.GeneratedMessage {
  factory EyeLandmarks({
    $core.Iterable<Point2D>? left,
    $core.Iterable<Point2D>? right,
  }) {
    final $result = create();
    if (left != null) {
      $result.left.addAll(left);
    }
    if (right != null) {
      $result.right.addAll(right);
    }
    return $result;
  }
  EyeLandmarks._() : super();
  factory EyeLandmarks.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory EyeLandmarks.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(_omitMessageNames ? '' : 'EyeLandmarks', package: const $pb.PackageName(_omitMessageNames ? '' : 'eyetrack.proto'), createEmptyInstance: create)
    ..pc<Point2D>(1, _omitFieldNames ? '' : 'left', $pb.PbFieldType.PM, subBuilder: Point2D.create)
    ..pc<Point2D>(2, _omitFieldNames ? '' : 'right', $pb.PbFieldType.PM, subBuilder: Point2D.create)
    ..hasRequiredFields = false
  ;

  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  EyeLandmarks clone() => EyeLandmarks()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  EyeLandmarks copyWith(void Function(EyeLandmarks) updates) => super.copyWith((message) => updates(message as EyeLandmarks)) as EyeLandmarks;

  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  static EyeLandmarks create() => EyeLandmarks._();
  EyeLandmarks createEmptyInstance() => create();
  static $pb.PbList<EyeLandmarks> createRepeated() => $pb.PbList<EyeLandmarks>();
  @$core.pragma('dart2js:noInline')
  static EyeLandmarks getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<EyeLandmarks>(create);
  static EyeLandmarks? _defaultInstance;

  @$pb.TagNumber(1)
  $core.List<Point2D> get left => $_getList(0);

  @$pb.TagNumber(2)
  $core.List<Point2D> get right => $_getList(1);
}

class PupilInfo extends $pb.GeneratedMessage {
  factory PupilInfo({
    Point2D? center,
    $core.double? radius,
    $core.double? confidence,
  }) {
    final $result = create();
    if (center != null) {
      $result.center = center;
    }
    if (radius != null) {
      $result.radius = radius;
    }
    if (confidence != null) {
      $result.confidence = confidence;
    }
    return $result;
  }
  PupilInfo._() : super();
  factory PupilInfo.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory PupilInfo.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(_omitMessageNames ? '' : 'PupilInfo', package: const $pb.PackageName(_omitMessageNames ? '' : 'eyetrack.proto'), createEmptyInstance: create)
    ..aOM<Point2D>(1, _omitFieldNames ? '' : 'center', subBuilder: Point2D.create)
    ..a<$core.double>(2, _omitFieldNames ? '' : 'radius', $pb.PbFieldType.OF)
    ..a<$core.double>(3, _omitFieldNames ? '' : 'confidence', $pb.PbFieldType.OF)
    ..hasRequiredFields = false
  ;

  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  PupilInfo clone() => PupilInfo()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  PupilInfo copyWith(void Function(PupilInfo) updates) => super.copyWith((message) => updates(message as PupilInfo)) as PupilInfo;

  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  static PupilInfo create() => PupilInfo._();
  PupilInfo createEmptyInstance() => create();
  static $pb.PbList<PupilInfo> createRepeated() => $pb.PbList<PupilInfo>();
  @$core.pragma('dart2js:noInline')
  static PupilInfo getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<PupilInfo>(create);
  static PupilInfo? _defaultInstance;

  @$pb.TagNumber(1)
  Point2D get center => $_getN(0);
  @$pb.TagNumber(1)
  set center(Point2D v) { setField(1, v); }
  @$pb.TagNumber(1)
  $core.bool hasCenter() => $_has(0);
  @$pb.TagNumber(1)
  void clearCenter() => clearField(1);
  @$pb.TagNumber(1)
  Point2D ensureCenter() => $_ensure(0);

  @$pb.TagNumber(2)
  $core.double get radius => $_getN(1);
  @$pb.TagNumber(2)
  set radius($core.double v) { $_setFloat(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasRadius() => $_has(1);
  @$pb.TagNumber(2)
  void clearRadius() => clearField(2);

  @$pb.TagNumber(3)
  $core.double get confidence => $_getN(2);
  @$pb.TagNumber(3)
  set confidence($core.double v) { $_setFloat(2, v); }
  @$pb.TagNumber(3)
  $core.bool hasConfidence() => $_has(2);
  @$pb.TagNumber(3)
  void clearConfidence() => clearField(3);
}

class Rect2f extends $pb.GeneratedMessage {
  factory Rect2f({
    $core.double? x,
    $core.double? y,
    $core.double? width,
    $core.double? height,
  }) {
    final $result = create();
    if (x != null) {
      $result.x = x;
    }
    if (y != null) {
      $result.y = y;
    }
    if (width != null) {
      $result.width = width;
    }
    if (height != null) {
      $result.height = height;
    }
    return $result;
  }
  Rect2f._() : super();
  factory Rect2f.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory Rect2f.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(_omitMessageNames ? '' : 'Rect2f', package: const $pb.PackageName(_omitMessageNames ? '' : 'eyetrack.proto'), createEmptyInstance: create)
    ..a<$core.double>(1, _omitFieldNames ? '' : 'x', $pb.PbFieldType.OF)
    ..a<$core.double>(2, _omitFieldNames ? '' : 'y', $pb.PbFieldType.OF)
    ..a<$core.double>(3, _omitFieldNames ? '' : 'width', $pb.PbFieldType.OF)
    ..a<$core.double>(4, _omitFieldNames ? '' : 'height', $pb.PbFieldType.OF)
    ..hasRequiredFields = false
  ;

  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  Rect2f clone() => Rect2f()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  Rect2f copyWith(void Function(Rect2f) updates) => super.copyWith((message) => updates(message as Rect2f)) as Rect2f;

  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  static Rect2f create() => Rect2f._();
  Rect2f createEmptyInstance() => create();
  static $pb.PbList<Rect2f> createRepeated() => $pb.PbList<Rect2f>();
  @$core.pragma('dart2js:noInline')
  static Rect2f getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<Rect2f>(create);
  static Rect2f? _defaultInstance;

  @$pb.TagNumber(1)
  $core.double get x => $_getN(0);
  @$pb.TagNumber(1)
  set x($core.double v) { $_setFloat(0, v); }
  @$pb.TagNumber(1)
  $core.bool hasX() => $_has(0);
  @$pb.TagNumber(1)
  void clearX() => clearField(1);

  @$pb.TagNumber(2)
  $core.double get y => $_getN(1);
  @$pb.TagNumber(2)
  set y($core.double v) { $_setFloat(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasY() => $_has(1);
  @$pb.TagNumber(2)
  void clearY() => clearField(2);

  @$pb.TagNumber(3)
  $core.double get width => $_getN(2);
  @$pb.TagNumber(3)
  set width($core.double v) { $_setFloat(2, v); }
  @$pb.TagNumber(3)
  $core.bool hasWidth() => $_has(2);
  @$pb.TagNumber(3)
  void clearWidth() => clearField(3);

  @$pb.TagNumber(4)
  $core.double get height => $_getN(3);
  @$pb.TagNumber(4)
  set height($core.double v) { $_setFloat(3, v); }
  @$pb.TagNumber(4)
  $core.bool hasHeight() => $_has(3);
  @$pb.TagNumber(4)
  void clearHeight() => clearField(4);
}

class FaceROI extends $pb.GeneratedMessage {
  factory FaceROI({
    Rect2f? boundingBox,
    $core.Iterable<Point2D>? landmarks,
    $core.double? confidence,
  }) {
    final $result = create();
    if (boundingBox != null) {
      $result.boundingBox = boundingBox;
    }
    if (landmarks != null) {
      $result.landmarks.addAll(landmarks);
    }
    if (confidence != null) {
      $result.confidence = confidence;
    }
    return $result;
  }
  FaceROI._() : super();
  factory FaceROI.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory FaceROI.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(_omitMessageNames ? '' : 'FaceROI', package: const $pb.PackageName(_omitMessageNames ? '' : 'eyetrack.proto'), createEmptyInstance: create)
    ..aOM<Rect2f>(1, _omitFieldNames ? '' : 'boundingBox', subBuilder: Rect2f.create)
    ..pc<Point2D>(2, _omitFieldNames ? '' : 'landmarks', $pb.PbFieldType.PM, subBuilder: Point2D.create)
    ..a<$core.double>(3, _omitFieldNames ? '' : 'confidence', $pb.PbFieldType.OF)
    ..hasRequiredFields = false
  ;

  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  FaceROI clone() => FaceROI()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  FaceROI copyWith(void Function(FaceROI) updates) => super.copyWith((message) => updates(message as FaceROI)) as FaceROI;

  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  static FaceROI create() => FaceROI._();
  FaceROI createEmptyInstance() => create();
  static $pb.PbList<FaceROI> createRepeated() => $pb.PbList<FaceROI>();
  @$core.pragma('dart2js:noInline')
  static FaceROI getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<FaceROI>(create);
  static FaceROI? _defaultInstance;

  @$pb.TagNumber(1)
  Rect2f get boundingBox => $_getN(0);
  @$pb.TagNumber(1)
  set boundingBox(Rect2f v) { setField(1, v); }
  @$pb.TagNumber(1)
  $core.bool hasBoundingBox() => $_has(0);
  @$pb.TagNumber(1)
  void clearBoundingBox() => clearField(1);
  @$pb.TagNumber(1)
  Rect2f ensureBoundingBox() => $_ensure(0);

  @$pb.TagNumber(2)
  $core.List<Point2D> get landmarks => $_getList(1);

  @$pb.TagNumber(3)
  $core.double get confidence => $_getN(2);
  @$pb.TagNumber(3)
  set confidence($core.double v) { $_setFloat(2, v); }
  @$pb.TagNumber(3)
  $core.bool hasConfidence() => $_has(2);
  @$pb.TagNumber(3)
  void clearConfidence() => clearField(3);
}

class FrameData extends $pb.GeneratedMessage {
  factory FrameData({
    $core.List<$core.int>? frame,
    $fixnum.Int64? frameId,
    $fixnum.Int64? timestampNs,
    $core.String? clientId,
    $core.int? width,
    $core.int? height,
    $core.int? channels,
  }) {
    final $result = create();
    if (frame != null) {
      $result.frame = frame;
    }
    if (frameId != null) {
      $result.frameId = frameId;
    }
    if (timestampNs != null) {
      $result.timestampNs = timestampNs;
    }
    if (clientId != null) {
      $result.clientId = clientId;
    }
    if (width != null) {
      $result.width = width;
    }
    if (height != null) {
      $result.height = height;
    }
    if (channels != null) {
      $result.channels = channels;
    }
    return $result;
  }
  FrameData._() : super();
  factory FrameData.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory FrameData.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(_omitMessageNames ? '' : 'FrameData', package: const $pb.PackageName(_omitMessageNames ? '' : 'eyetrack.proto'), createEmptyInstance: create)
    ..a<$core.List<$core.int>>(1, _omitFieldNames ? '' : 'frame', $pb.PbFieldType.OY)
    ..a<$fixnum.Int64>(2, _omitFieldNames ? '' : 'frameId', $pb.PbFieldType.OU6, defaultOrMaker: $fixnum.Int64.ZERO)
    ..aInt64(3, _omitFieldNames ? '' : 'timestampNs')
    ..aOS(4, _omitFieldNames ? '' : 'clientId')
    ..a<$core.int>(5, _omitFieldNames ? '' : 'width', $pb.PbFieldType.OU3)
    ..a<$core.int>(6, _omitFieldNames ? '' : 'height', $pb.PbFieldType.OU3)
    ..a<$core.int>(7, _omitFieldNames ? '' : 'channels', $pb.PbFieldType.OU3)
    ..hasRequiredFields = false
  ;

  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  FrameData clone() => FrameData()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  FrameData copyWith(void Function(FrameData) updates) => super.copyWith((message) => updates(message as FrameData)) as FrameData;

  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  static FrameData create() => FrameData._();
  FrameData createEmptyInstance() => create();
  static $pb.PbList<FrameData> createRepeated() => $pb.PbList<FrameData>();
  @$core.pragma('dart2js:noInline')
  static FrameData getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<FrameData>(create);
  static FrameData? _defaultInstance;

  @$pb.TagNumber(1)
  $core.List<$core.int> get frame => $_getN(0);
  @$pb.TagNumber(1)
  set frame($core.List<$core.int> v) { $_setBytes(0, v); }
  @$pb.TagNumber(1)
  $core.bool hasFrame() => $_has(0);
  @$pb.TagNumber(1)
  void clearFrame() => clearField(1);

  @$pb.TagNumber(2)
  $fixnum.Int64 get frameId => $_getI64(1);
  @$pb.TagNumber(2)
  set frameId($fixnum.Int64 v) { $_setInt64(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasFrameId() => $_has(1);
  @$pb.TagNumber(2)
  void clearFrameId() => clearField(2);

  @$pb.TagNumber(3)
  $fixnum.Int64 get timestampNs => $_getI64(2);
  @$pb.TagNumber(3)
  set timestampNs($fixnum.Int64 v) { $_setInt64(2, v); }
  @$pb.TagNumber(3)
  $core.bool hasTimestampNs() => $_has(2);
  @$pb.TagNumber(3)
  void clearTimestampNs() => clearField(3);

  @$pb.TagNumber(4)
  $core.String get clientId => $_getSZ(3);
  @$pb.TagNumber(4)
  set clientId($core.String v) { $_setString(3, v); }
  @$pb.TagNumber(4)
  $core.bool hasClientId() => $_has(3);
  @$pb.TagNumber(4)
  void clearClientId() => clearField(4);

  @$pb.TagNumber(5)
  $core.int get width => $_getIZ(4);
  @$pb.TagNumber(5)
  set width($core.int v) { $_setUnsignedInt32(4, v); }
  @$pb.TagNumber(5)
  $core.bool hasWidth() => $_has(4);
  @$pb.TagNumber(5)
  void clearWidth() => clearField(5);

  @$pb.TagNumber(6)
  $core.int get height => $_getIZ(5);
  @$pb.TagNumber(6)
  set height($core.int v) { $_setUnsignedInt32(5, v); }
  @$pb.TagNumber(6)
  $core.bool hasHeight() => $_has(5);
  @$pb.TagNumber(6)
  void clearHeight() => clearField(6);

  @$pb.TagNumber(7)
  $core.int get channels => $_getIZ(6);
  @$pb.TagNumber(7)
  set channels($core.int v) { $_setUnsignedInt32(6, v); }
  @$pb.TagNumber(7)
  $core.bool hasChannels() => $_has(6);
  @$pb.TagNumber(7)
  void clearChannels() => clearField(7);
}

class FeatureData extends $pb.GeneratedMessage {
  factory FeatureData({
    EyeLandmarks? eyeLandmarks,
    PupilInfo? leftPupil,
    PupilInfo? rightPupil,
    $core.double? earLeft,
    $core.double? earRight,
    FaceROI? faceRoi,
    $fixnum.Int64? frameId,
  }) {
    final $result = create();
    if (eyeLandmarks != null) {
      $result.eyeLandmarks = eyeLandmarks;
    }
    if (leftPupil != null) {
      $result.leftPupil = leftPupil;
    }
    if (rightPupil != null) {
      $result.rightPupil = rightPupil;
    }
    if (earLeft != null) {
      $result.earLeft = earLeft;
    }
    if (earRight != null) {
      $result.earRight = earRight;
    }
    if (faceRoi != null) {
      $result.faceRoi = faceRoi;
    }
    if (frameId != null) {
      $result.frameId = frameId;
    }
    return $result;
  }
  FeatureData._() : super();
  factory FeatureData.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory FeatureData.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(_omitMessageNames ? '' : 'FeatureData', package: const $pb.PackageName(_omitMessageNames ? '' : 'eyetrack.proto'), createEmptyInstance: create)
    ..aOM<EyeLandmarks>(1, _omitFieldNames ? '' : 'eyeLandmarks', subBuilder: EyeLandmarks.create)
    ..aOM<PupilInfo>(2, _omitFieldNames ? '' : 'leftPupil', subBuilder: PupilInfo.create)
    ..aOM<PupilInfo>(3, _omitFieldNames ? '' : 'rightPupil', subBuilder: PupilInfo.create)
    ..a<$core.double>(4, _omitFieldNames ? '' : 'earLeft', $pb.PbFieldType.OF)
    ..a<$core.double>(5, _omitFieldNames ? '' : 'earRight', $pb.PbFieldType.OF)
    ..aOM<FaceROI>(6, _omitFieldNames ? '' : 'faceRoi', subBuilder: FaceROI.create)
    ..a<$fixnum.Int64>(7, _omitFieldNames ? '' : 'frameId', $pb.PbFieldType.OU6, defaultOrMaker: $fixnum.Int64.ZERO)
    ..hasRequiredFields = false
  ;

  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  FeatureData clone() => FeatureData()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  FeatureData copyWith(void Function(FeatureData) updates) => super.copyWith((message) => updates(message as FeatureData)) as FeatureData;

  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  static FeatureData create() => FeatureData._();
  FeatureData createEmptyInstance() => create();
  static $pb.PbList<FeatureData> createRepeated() => $pb.PbList<FeatureData>();
  @$core.pragma('dart2js:noInline')
  static FeatureData getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<FeatureData>(create);
  static FeatureData? _defaultInstance;

  @$pb.TagNumber(1)
  EyeLandmarks get eyeLandmarks => $_getN(0);
  @$pb.TagNumber(1)
  set eyeLandmarks(EyeLandmarks v) { setField(1, v); }
  @$pb.TagNumber(1)
  $core.bool hasEyeLandmarks() => $_has(0);
  @$pb.TagNumber(1)
  void clearEyeLandmarks() => clearField(1);
  @$pb.TagNumber(1)
  EyeLandmarks ensureEyeLandmarks() => $_ensure(0);

  @$pb.TagNumber(2)
  PupilInfo get leftPupil => $_getN(1);
  @$pb.TagNumber(2)
  set leftPupil(PupilInfo v) { setField(2, v); }
  @$pb.TagNumber(2)
  $core.bool hasLeftPupil() => $_has(1);
  @$pb.TagNumber(2)
  void clearLeftPupil() => clearField(2);
  @$pb.TagNumber(2)
  PupilInfo ensureLeftPupil() => $_ensure(1);

  @$pb.TagNumber(3)
  PupilInfo get rightPupil => $_getN(2);
  @$pb.TagNumber(3)
  set rightPupil(PupilInfo v) { setField(3, v); }
  @$pb.TagNumber(3)
  $core.bool hasRightPupil() => $_has(2);
  @$pb.TagNumber(3)
  void clearRightPupil() => clearField(3);
  @$pb.TagNumber(3)
  PupilInfo ensureRightPupil() => $_ensure(2);

  @$pb.TagNumber(4)
  $core.double get earLeft => $_getN(3);
  @$pb.TagNumber(4)
  set earLeft($core.double v) { $_setFloat(3, v); }
  @$pb.TagNumber(4)
  $core.bool hasEarLeft() => $_has(3);
  @$pb.TagNumber(4)
  void clearEarLeft() => clearField(4);

  @$pb.TagNumber(5)
  $core.double get earRight => $_getN(4);
  @$pb.TagNumber(5)
  set earRight($core.double v) { $_setFloat(4, v); }
  @$pb.TagNumber(5)
  $core.bool hasEarRight() => $_has(4);
  @$pb.TagNumber(5)
  void clearEarRight() => clearField(5);

  @$pb.TagNumber(6)
  FaceROI get faceRoi => $_getN(5);
  @$pb.TagNumber(6)
  set faceRoi(FaceROI v) { setField(6, v); }
  @$pb.TagNumber(6)
  $core.bool hasFaceRoi() => $_has(5);
  @$pb.TagNumber(6)
  void clearFaceRoi() => clearField(6);
  @$pb.TagNumber(6)
  FaceROI ensureFaceRoi() => $_ensure(5);

  @$pb.TagNumber(7)
  $fixnum.Int64 get frameId => $_getI64(6);
  @$pb.TagNumber(7)
  set frameId($fixnum.Int64 v) { $_setInt64(6, v); }
  @$pb.TagNumber(7)
  $core.bool hasFrameId() => $_has(6);
  @$pb.TagNumber(7)
  void clearFrameId() => clearField(7);
}

class GazeEvent extends $pb.GeneratedMessage {
  factory GazeEvent({
    Point2D? gazePoint,
    $core.double? confidence,
    BlinkType? blink,
    $fixnum.Int64? timestampNs,
    $core.String? clientId,
    $fixnum.Int64? frameId,
  }) {
    final $result = create();
    if (gazePoint != null) {
      $result.gazePoint = gazePoint;
    }
    if (confidence != null) {
      $result.confidence = confidence;
    }
    if (blink != null) {
      $result.blink = blink;
    }
    if (timestampNs != null) {
      $result.timestampNs = timestampNs;
    }
    if (clientId != null) {
      $result.clientId = clientId;
    }
    if (frameId != null) {
      $result.frameId = frameId;
    }
    return $result;
  }
  GazeEvent._() : super();
  factory GazeEvent.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory GazeEvent.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(_omitMessageNames ? '' : 'GazeEvent', package: const $pb.PackageName(_omitMessageNames ? '' : 'eyetrack.proto'), createEmptyInstance: create)
    ..aOM<Point2D>(1, _omitFieldNames ? '' : 'gazePoint', subBuilder: Point2D.create)
    ..a<$core.double>(2, _omitFieldNames ? '' : 'confidence', $pb.PbFieldType.OF)
    ..e<BlinkType>(3, _omitFieldNames ? '' : 'blink', $pb.PbFieldType.OE, defaultOrMaker: BlinkType.BLINK_NONE, valueOf: BlinkType.valueOf, enumValues: BlinkType.values)
    ..aInt64(4, _omitFieldNames ? '' : 'timestampNs')
    ..aOS(5, _omitFieldNames ? '' : 'clientId')
    ..a<$fixnum.Int64>(6, _omitFieldNames ? '' : 'frameId', $pb.PbFieldType.OU6, defaultOrMaker: $fixnum.Int64.ZERO)
    ..hasRequiredFields = false
  ;

  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  GazeEvent clone() => GazeEvent()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  GazeEvent copyWith(void Function(GazeEvent) updates) => super.copyWith((message) => updates(message as GazeEvent)) as GazeEvent;

  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  static GazeEvent create() => GazeEvent._();
  GazeEvent createEmptyInstance() => create();
  static $pb.PbList<GazeEvent> createRepeated() => $pb.PbList<GazeEvent>();
  @$core.pragma('dart2js:noInline')
  static GazeEvent getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<GazeEvent>(create);
  static GazeEvent? _defaultInstance;

  @$pb.TagNumber(1)
  Point2D get gazePoint => $_getN(0);
  @$pb.TagNumber(1)
  set gazePoint(Point2D v) { setField(1, v); }
  @$pb.TagNumber(1)
  $core.bool hasGazePoint() => $_has(0);
  @$pb.TagNumber(1)
  void clearGazePoint() => clearField(1);
  @$pb.TagNumber(1)
  Point2D ensureGazePoint() => $_ensure(0);

  @$pb.TagNumber(2)
  $core.double get confidence => $_getN(1);
  @$pb.TagNumber(2)
  set confidence($core.double v) { $_setFloat(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasConfidence() => $_has(1);
  @$pb.TagNumber(2)
  void clearConfidence() => clearField(2);

  @$pb.TagNumber(3)
  BlinkType get blink => $_getN(2);
  @$pb.TagNumber(3)
  set blink(BlinkType v) { setField(3, v); }
  @$pb.TagNumber(3)
  $core.bool hasBlink() => $_has(2);
  @$pb.TagNumber(3)
  void clearBlink() => clearField(3);

  @$pb.TagNumber(4)
  $fixnum.Int64 get timestampNs => $_getI64(3);
  @$pb.TagNumber(4)
  set timestampNs($fixnum.Int64 v) { $_setInt64(3, v); }
  @$pb.TagNumber(4)
  $core.bool hasTimestampNs() => $_has(3);
  @$pb.TagNumber(4)
  void clearTimestampNs() => clearField(4);

  @$pb.TagNumber(5)
  $core.String get clientId => $_getSZ(4);
  @$pb.TagNumber(5)
  set clientId($core.String v) { $_setString(4, v); }
  @$pb.TagNumber(5)
  $core.bool hasClientId() => $_has(4);
  @$pb.TagNumber(5)
  void clearClientId() => clearField(5);

  @$pb.TagNumber(6)
  $fixnum.Int64 get frameId => $_getI64(5);
  @$pb.TagNumber(6)
  set frameId($fixnum.Int64 v) { $_setInt64(5, v); }
  @$pb.TagNumber(6)
  $core.bool hasFrameId() => $_has(5);
  @$pb.TagNumber(6)
  void clearFrameId() => clearField(6);
}

class CalibrationPoint extends $pb.GeneratedMessage {
  factory CalibrationPoint({
    Point2D? screenPoint,
    PupilInfo? pupilObservation,
  }) {
    final $result = create();
    if (screenPoint != null) {
      $result.screenPoint = screenPoint;
    }
    if (pupilObservation != null) {
      $result.pupilObservation = pupilObservation;
    }
    return $result;
  }
  CalibrationPoint._() : super();
  factory CalibrationPoint.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory CalibrationPoint.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(_omitMessageNames ? '' : 'CalibrationPoint', package: const $pb.PackageName(_omitMessageNames ? '' : 'eyetrack.proto'), createEmptyInstance: create)
    ..aOM<Point2D>(1, _omitFieldNames ? '' : 'screenPoint', subBuilder: Point2D.create)
    ..aOM<PupilInfo>(2, _omitFieldNames ? '' : 'pupilObservation', subBuilder: PupilInfo.create)
    ..hasRequiredFields = false
  ;

  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  CalibrationPoint clone() => CalibrationPoint()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  CalibrationPoint copyWith(void Function(CalibrationPoint) updates) => super.copyWith((message) => updates(message as CalibrationPoint)) as CalibrationPoint;

  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  static CalibrationPoint create() => CalibrationPoint._();
  CalibrationPoint createEmptyInstance() => create();
  static $pb.PbList<CalibrationPoint> createRepeated() => $pb.PbList<CalibrationPoint>();
  @$core.pragma('dart2js:noInline')
  static CalibrationPoint getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<CalibrationPoint>(create);
  static CalibrationPoint? _defaultInstance;

  @$pb.TagNumber(1)
  Point2D get screenPoint => $_getN(0);
  @$pb.TagNumber(1)
  set screenPoint(Point2D v) { setField(1, v); }
  @$pb.TagNumber(1)
  $core.bool hasScreenPoint() => $_has(0);
  @$pb.TagNumber(1)
  void clearScreenPoint() => clearField(1);
  @$pb.TagNumber(1)
  Point2D ensureScreenPoint() => $_ensure(0);

  @$pb.TagNumber(2)
  PupilInfo get pupilObservation => $_getN(1);
  @$pb.TagNumber(2)
  set pupilObservation(PupilInfo v) { setField(2, v); }
  @$pb.TagNumber(2)
  $core.bool hasPupilObservation() => $_has(1);
  @$pb.TagNumber(2)
  void clearPupilObservation() => clearField(2);
  @$pb.TagNumber(2)
  PupilInfo ensurePupilObservation() => $_ensure(1);
}

class CalibrationRequest extends $pb.GeneratedMessage {
  factory CalibrationRequest({
    $core.String? userId,
    $core.Iterable<CalibrationPoint>? points,
  }) {
    final $result = create();
    if (userId != null) {
      $result.userId = userId;
    }
    if (points != null) {
      $result.points.addAll(points);
    }
    return $result;
  }
  CalibrationRequest._() : super();
  factory CalibrationRequest.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory CalibrationRequest.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(_omitMessageNames ? '' : 'CalibrationRequest', package: const $pb.PackageName(_omitMessageNames ? '' : 'eyetrack.proto'), createEmptyInstance: create)
    ..aOS(1, _omitFieldNames ? '' : 'userId')
    ..pc<CalibrationPoint>(2, _omitFieldNames ? '' : 'points', $pb.PbFieldType.PM, subBuilder: CalibrationPoint.create)
    ..hasRequiredFields = false
  ;

  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  CalibrationRequest clone() => CalibrationRequest()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  CalibrationRequest copyWith(void Function(CalibrationRequest) updates) => super.copyWith((message) => updates(message as CalibrationRequest)) as CalibrationRequest;

  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  static CalibrationRequest create() => CalibrationRequest._();
  CalibrationRequest createEmptyInstance() => create();
  static $pb.PbList<CalibrationRequest> createRepeated() => $pb.PbList<CalibrationRequest>();
  @$core.pragma('dart2js:noInline')
  static CalibrationRequest getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<CalibrationRequest>(create);
  static CalibrationRequest? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get userId => $_getSZ(0);
  @$pb.TagNumber(1)
  set userId($core.String v) { $_setString(0, v); }
  @$pb.TagNumber(1)
  $core.bool hasUserId() => $_has(0);
  @$pb.TagNumber(1)
  void clearUserId() => clearField(1);

  @$pb.TagNumber(2)
  $core.List<CalibrationPoint> get points => $_getList(1);
}

class CalibrationResponse extends $pb.GeneratedMessage {
  factory CalibrationResponse({
    $core.bool? success,
    $core.String? errorMessage,
    $core.String? profileId,
  }) {
    final $result = create();
    if (success != null) {
      $result.success = success;
    }
    if (errorMessage != null) {
      $result.errorMessage = errorMessage;
    }
    if (profileId != null) {
      $result.profileId = profileId;
    }
    return $result;
  }
  CalibrationResponse._() : super();
  factory CalibrationResponse.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory CalibrationResponse.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(_omitMessageNames ? '' : 'CalibrationResponse', package: const $pb.PackageName(_omitMessageNames ? '' : 'eyetrack.proto'), createEmptyInstance: create)
    ..aOB(1, _omitFieldNames ? '' : 'success')
    ..aOS(2, _omitFieldNames ? '' : 'errorMessage')
    ..aOS(3, _omitFieldNames ? '' : 'profileId')
    ..hasRequiredFields = false
  ;

  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  CalibrationResponse clone() => CalibrationResponse()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  CalibrationResponse copyWith(void Function(CalibrationResponse) updates) => super.copyWith((message) => updates(message as CalibrationResponse)) as CalibrationResponse;

  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  static CalibrationResponse create() => CalibrationResponse._();
  CalibrationResponse createEmptyInstance() => create();
  static $pb.PbList<CalibrationResponse> createRepeated() => $pb.PbList<CalibrationResponse>();
  @$core.pragma('dart2js:noInline')
  static CalibrationResponse getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<CalibrationResponse>(create);
  static CalibrationResponse? _defaultInstance;

  @$pb.TagNumber(1)
  $core.bool get success => $_getBF(0);
  @$pb.TagNumber(1)
  set success($core.bool v) { $_setBool(0, v); }
  @$pb.TagNumber(1)
  $core.bool hasSuccess() => $_has(0);
  @$pb.TagNumber(1)
  void clearSuccess() => clearField(1);

  @$pb.TagNumber(2)
  $core.String get errorMessage => $_getSZ(1);
  @$pb.TagNumber(2)
  set errorMessage($core.String v) { $_setString(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasErrorMessage() => $_has(1);
  @$pb.TagNumber(2)
  void clearErrorMessage() => clearField(2);

  @$pb.TagNumber(3)
  $core.String get profileId => $_getSZ(2);
  @$pb.TagNumber(3)
  set profileId($core.String v) { $_setString(2, v); }
  @$pb.TagNumber(3)
  $core.bool hasProfileId() => $_has(2);
  @$pb.TagNumber(3)
  void clearProfileId() => clearField(3);
}

class StreamGazeRequest extends $pb.GeneratedMessage {
  factory StreamGazeRequest({
    $core.String? clientId,
  }) {
    final $result = create();
    if (clientId != null) {
      $result.clientId = clientId;
    }
    return $result;
  }
  StreamGazeRequest._() : super();
  factory StreamGazeRequest.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory StreamGazeRequest.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(_omitMessageNames ? '' : 'StreamGazeRequest', package: const $pb.PackageName(_omitMessageNames ? '' : 'eyetrack.proto'), createEmptyInstance: create)
    ..aOS(1, _omitFieldNames ? '' : 'clientId')
    ..hasRequiredFields = false
  ;

  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  StreamGazeRequest clone() => StreamGazeRequest()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  StreamGazeRequest copyWith(void Function(StreamGazeRequest) updates) => super.copyWith((message) => updates(message as StreamGazeRequest)) as StreamGazeRequest;

  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  static StreamGazeRequest create() => StreamGazeRequest._();
  StreamGazeRequest createEmptyInstance() => create();
  static $pb.PbList<StreamGazeRequest> createRepeated() => $pb.PbList<StreamGazeRequest>();
  @$core.pragma('dart2js:noInline')
  static StreamGazeRequest getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<StreamGazeRequest>(create);
  static StreamGazeRequest? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get clientId => $_getSZ(0);
  @$pb.TagNumber(1)
  set clientId($core.String v) { $_setString(0, v); }
  @$pb.TagNumber(1)
  $core.bool hasClientId() => $_has(0);
  @$pb.TagNumber(1)
  void clearClientId() => clearField(1);
}

class StatusRequest extends $pb.GeneratedMessage {
  factory StatusRequest() => create();
  StatusRequest._() : super();
  factory StatusRequest.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory StatusRequest.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(_omitMessageNames ? '' : 'StatusRequest', package: const $pb.PackageName(_omitMessageNames ? '' : 'eyetrack.proto'), createEmptyInstance: create)
    ..hasRequiredFields = false
  ;

  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  StatusRequest clone() => StatusRequest()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  StatusRequest copyWith(void Function(StatusRequest) updates) => super.copyWith((message) => updates(message as StatusRequest)) as StatusRequest;

  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  static StatusRequest create() => StatusRequest._();
  StatusRequest createEmptyInstance() => create();
  static $pb.PbList<StatusRequest> createRepeated() => $pb.PbList<StatusRequest>();
  @$core.pragma('dart2js:noInline')
  static StatusRequest getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<StatusRequest>(create);
  static StatusRequest? _defaultInstance;
}

class StatusResponse extends $pb.GeneratedMessage {
  factory StatusResponse({
    $core.bool? isRunning,
    $core.int? connectedClients,
    $core.double? fps,
    $core.String? version,
  }) {
    final $result = create();
    if (isRunning != null) {
      $result.isRunning = isRunning;
    }
    if (connectedClients != null) {
      $result.connectedClients = connectedClients;
    }
    if (fps != null) {
      $result.fps = fps;
    }
    if (version != null) {
      $result.version = version;
    }
    return $result;
  }
  StatusResponse._() : super();
  factory StatusResponse.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory StatusResponse.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(_omitMessageNames ? '' : 'StatusResponse', package: const $pb.PackageName(_omitMessageNames ? '' : 'eyetrack.proto'), createEmptyInstance: create)
    ..aOB(1, _omitFieldNames ? '' : 'isRunning')
    ..a<$core.int>(2, _omitFieldNames ? '' : 'connectedClients', $pb.PbFieldType.OU3)
    ..a<$core.double>(3, _omitFieldNames ? '' : 'fps', $pb.PbFieldType.OF)
    ..aOS(4, _omitFieldNames ? '' : 'version')
    ..hasRequiredFields = false
  ;

  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  StatusResponse clone() => StatusResponse()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  StatusResponse copyWith(void Function(StatusResponse) updates) => super.copyWith((message) => updates(message as StatusResponse)) as StatusResponse;

  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  static StatusResponse create() => StatusResponse._();
  StatusResponse createEmptyInstance() => create();
  static $pb.PbList<StatusResponse> createRepeated() => $pb.PbList<StatusResponse>();
  @$core.pragma('dart2js:noInline')
  static StatusResponse getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<StatusResponse>(create);
  static StatusResponse? _defaultInstance;

  @$pb.TagNumber(1)
  $core.bool get isRunning => $_getBF(0);
  @$pb.TagNumber(1)
  set isRunning($core.bool v) { $_setBool(0, v); }
  @$pb.TagNumber(1)
  $core.bool hasIsRunning() => $_has(0);
  @$pb.TagNumber(1)
  void clearIsRunning() => clearField(1);

  @$pb.TagNumber(2)
  $core.int get connectedClients => $_getIZ(1);
  @$pb.TagNumber(2)
  set connectedClients($core.int v) { $_setUnsignedInt32(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasConnectedClients() => $_has(1);
  @$pb.TagNumber(2)
  void clearConnectedClients() => clearField(2);

  @$pb.TagNumber(3)
  $core.double get fps => $_getN(2);
  @$pb.TagNumber(3)
  set fps($core.double v) { $_setFloat(2, v); }
  @$pb.TagNumber(3)
  $core.bool hasFps() => $_has(2);
  @$pb.TagNumber(3)
  void clearFps() => clearField(3);

  @$pb.TagNumber(4)
  $core.String get version => $_getSZ(3);
  @$pb.TagNumber(4)
  set version($core.String v) { $_setString(3, v); }
  @$pb.TagNumber(4)
  $core.bool hasVersion() => $_has(3);
  @$pb.TagNumber(4)
  void clearVersion() => clearField(4);
}


const _omitFieldNames = $core.bool.fromEnvironment('protobuf.omit_field_names');
const _omitMessageNames = $core.bool.fromEnvironment('protobuf.omit_message_names');
