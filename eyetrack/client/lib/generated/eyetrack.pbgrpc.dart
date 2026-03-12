//
//  Generated code. Do not modify.
//  source: eyetrack.proto
//
// @dart = 2.12

// ignore_for_file: annotate_overrides, camel_case_types, comment_references
// ignore_for_file: constant_identifier_names, library_prefixes
// ignore_for_file: non_constant_identifier_names, prefer_final_fields
// ignore_for_file: unnecessary_import, unnecessary_this, unused_import

import 'dart:async' as $async;
import 'dart:core' as $core;

import 'package:grpc/service_api.dart' as $grpc;
import 'package:protobuf/protobuf.dart' as $pb;

import 'eyetrack.pb.dart' as $0;

export 'eyetrack.pb.dart';

@$pb.GrpcServiceName('eyetrack.proto.EyeTrackService')
class EyeTrackServiceClient extends $grpc.Client {
  static final _$streamGaze = $grpc.ClientMethod<$0.StreamGazeRequest, $0.GazeEvent>(
      '/eyetrack.proto.EyeTrackService/StreamGaze',
      ($0.StreamGazeRequest value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $0.GazeEvent.fromBuffer(value));
  static final _$processFrame = $grpc.ClientMethod<$0.FrameData, $0.GazeEvent>(
      '/eyetrack.proto.EyeTrackService/ProcessFrame',
      ($0.FrameData value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $0.GazeEvent.fromBuffer(value));
  static final _$calibrate = $grpc.ClientMethod<$0.CalibrationRequest, $0.CalibrationResponse>(
      '/eyetrack.proto.EyeTrackService/Calibrate',
      ($0.CalibrationRequest value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $0.CalibrationResponse.fromBuffer(value));
  static final _$getStatus = $grpc.ClientMethod<$0.StatusRequest, $0.StatusResponse>(
      '/eyetrack.proto.EyeTrackService/GetStatus',
      ($0.StatusRequest value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $0.StatusResponse.fromBuffer(value));

  EyeTrackServiceClient($grpc.ClientChannel channel,
      {$grpc.CallOptions? options,
      $core.Iterable<$grpc.ClientInterceptor>? interceptors})
      : super(channel, options: options,
        interceptors: interceptors);

  $grpc.ResponseStream<$0.GazeEvent> streamGaze($0.StreamGazeRequest request, {$grpc.CallOptions? options}) {
    return $createStreamingCall(_$streamGaze, $async.Stream.fromIterable([request]), options: options);
  }

  $grpc.ResponseFuture<$0.GazeEvent> processFrame($0.FrameData request, {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$processFrame, request, options: options);
  }

  $grpc.ResponseFuture<$0.CalibrationResponse> calibrate($0.CalibrationRequest request, {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$calibrate, request, options: options);
  }

  $grpc.ResponseFuture<$0.StatusResponse> getStatus($0.StatusRequest request, {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$getStatus, request, options: options);
  }
}

@$pb.GrpcServiceName('eyetrack.proto.EyeTrackService')
abstract class EyeTrackServiceBase extends $grpc.Service {
  $core.String get $name => 'eyetrack.proto.EyeTrackService';

  EyeTrackServiceBase() {
    $addMethod($grpc.ServiceMethod<$0.StreamGazeRequest, $0.GazeEvent>(
        'StreamGaze',
        streamGaze_Pre,
        false,
        true,
        ($core.List<$core.int> value) => $0.StreamGazeRequest.fromBuffer(value),
        ($0.GazeEvent value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.FrameData, $0.GazeEvent>(
        'ProcessFrame',
        processFrame_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.FrameData.fromBuffer(value),
        ($0.GazeEvent value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.CalibrationRequest, $0.CalibrationResponse>(
        'Calibrate',
        calibrate_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.CalibrationRequest.fromBuffer(value),
        ($0.CalibrationResponse value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.StatusRequest, $0.StatusResponse>(
        'GetStatus',
        getStatus_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.StatusRequest.fromBuffer(value),
        ($0.StatusResponse value) => value.writeToBuffer()));
  }

  $async.Stream<$0.GazeEvent> streamGaze_Pre($grpc.ServiceCall call, $async.Future<$0.StreamGazeRequest> request) async* {
    yield* streamGaze(call, await request);
  }

  $async.Future<$0.GazeEvent> processFrame_Pre($grpc.ServiceCall call, $async.Future<$0.FrameData> request) async {
    return processFrame(call, await request);
  }

  $async.Future<$0.CalibrationResponse> calibrate_Pre($grpc.ServiceCall call, $async.Future<$0.CalibrationRequest> request) async {
    return calibrate(call, await request);
  }

  $async.Future<$0.StatusResponse> getStatus_Pre($grpc.ServiceCall call, $async.Future<$0.StatusRequest> request) async {
    return getStatus(call, await request);
  }

  $async.Stream<$0.GazeEvent> streamGaze($grpc.ServiceCall call, $0.StreamGazeRequest request);
  $async.Future<$0.GazeEvent> processFrame($grpc.ServiceCall call, $0.FrameData request);
  $async.Future<$0.CalibrationResponse> calibrate($grpc.ServiceCall call, $0.CalibrationRequest request);
  $async.Future<$0.StatusResponse> getStatus($grpc.ServiceCall call, $0.StatusRequest request);
}
