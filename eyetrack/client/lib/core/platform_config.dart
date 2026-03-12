import 'package:flutter/foundation.dart';

import 'package:eyetrack/data/eyetrack_client.dart';
import 'package:eyetrack/data/grpc_eyetrack_client.dart';
import 'package:eyetrack/data/mqtt_eyetrack_client.dart';
import 'package:eyetrack/data/websocket_eyetrack_client.dart';

enum Protocol { grpc, webSocket, mqtt }

Protocol detectPlatformProtocol() {
  if (kIsWeb) {
    return Protocol.webSocket;
  }

  switch (defaultTargetPlatform) {
    case TargetPlatform.iOS:
    case TargetPlatform.android:
    case TargetPlatform.macOS:
    case TargetPlatform.windows:
      return Protocol.grpc;
    case TargetPlatform.linux:
    case TargetPlatform.fuchsia:
      return Protocol.mqtt;
  }
}

EyeTrackClient createClient({
  required String host,
  Protocol? protocol,
  int? port,
  String? clientId,
  bool useTls = false,
}) {
  final effectiveProtocol = protocol ?? detectPlatformProtocol();

  switch (effectiveProtocol) {
    case Protocol.grpc:
      return GrpcEyeTrackClient(
        host: host,
        port: port ?? 50051,
        useTls: useTls,
      );
    case Protocol.webSocket:
      return WebSocketEyeTrackClient(
        host: host,
        port: port ?? 8080,
        useTls: useTls,
      );
    case Protocol.mqtt:
      return MqttEyeTrackClient(
        host: host,
        port: port ?? 1883,
        clientId: clientId ?? 'eyetrack_flutter',
      );
  }
}
