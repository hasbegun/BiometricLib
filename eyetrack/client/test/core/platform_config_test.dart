import 'package:flutter/foundation.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:eyetrack/core/platform_config.dart';
import 'package:eyetrack/data/eyetrack_client.dart';
import 'package:eyetrack/data/grpc_eyetrack_client.dart';
import 'package:eyetrack/data/mqtt_eyetrack_client.dart';
import 'package:eyetrack/data/websocket_eyetrack_client.dart';

void main() {
  group('Platform auto-detection', () {
    test('Protocol enum has three values', () {
      expect(Protocol.values.length, 3);
      expect(Protocol.grpc, isNotNull);
      expect(Protocol.webSocket, isNotNull);
      expect(Protocol.mqtt, isNotNull);
    });

    test('detectPlatformProtocol returns a valid protocol', () {
      final protocol = detectPlatformProtocol();
      expect(Protocol.values.contains(protocol), isTrue);
    });

    // In test environment (not web), default platform is typically linux or macOS
    test('non-web platform returns grpc or mqtt', () {
      // kIsWeb is false in test environment
      expect(kIsWeb, isFalse);
      final protocol = detectPlatformProtocol();
      // macOS -> grpc, linux -> mqtt
      expect(
        protocol == Protocol.grpc || protocol == Protocol.mqtt,
        isTrue,
      );
    });
  });

  group('createClient factory', () {
    test('creates GrpcEyeTrackClient when protocol is grpc', () {
      final client = createClient(
        host: 'localhost',
        protocol: Protocol.grpc,
      );
      expect(client, isA<GrpcEyeTrackClient>());
      expect(client, isA<EyeTrackClient>());
    });

    test('creates WebSocketEyeTrackClient when protocol is webSocket', () {
      final client = createClient(
        host: 'localhost',
        protocol: Protocol.webSocket,
      );
      expect(client, isA<WebSocketEyeTrackClient>());
      expect(client, isA<EyeTrackClient>());
    });

    test('creates MqttEyeTrackClient when protocol is mqtt', () {
      final client = createClient(
        host: 'localhost',
        protocol: Protocol.mqtt,
        clientId: 'test_device',
      );
      expect(client, isA<MqttEyeTrackClient>());
      expect(client, isA<EyeTrackClient>());
    });

    test('uses custom port when specified', () {
      final grpc = createClient(
        host: 'localhost',
        protocol: Protocol.grpc,
        port: 9999,
      ) as GrpcEyeTrackClient;
      expect(grpc.port, 9999);

      final ws = createClient(
        host: 'localhost',
        protocol: Protocol.webSocket,
        port: 7777,
      ) as WebSocketEyeTrackClient;
      expect(ws.port, 7777);

      final mqtt = createClient(
        host: 'localhost',
        protocol: Protocol.mqtt,
        port: 5555,
        clientId: 'test',
      ) as MqttEyeTrackClient;
      expect(mqtt.port, 5555);
    });

    test('uses default ports when not specified', () {
      final grpc = createClient(
        host: 'localhost',
        protocol: Protocol.grpc,
      ) as GrpcEyeTrackClient;
      expect(grpc.port, 50051);

      final ws = createClient(
        host: 'localhost',
        protocol: Protocol.webSocket,
      ) as WebSocketEyeTrackClient;
      expect(ws.port, 8080);

      final mqtt = createClient(
        host: 'localhost',
        protocol: Protocol.mqtt,
      ) as MqttEyeTrackClient;
      expect(mqtt.port, 1883);
    });

    test('passes TLS flag to grpc client', () {
      final client = createClient(
        host: 'localhost',
        protocol: Protocol.grpc,
        useTls: true,
      ) as GrpcEyeTrackClient;
      expect(client.useTls, isTrue);
    });

    test('passes TLS flag to websocket client', () {
      final client = createClient(
        host: 'localhost',
        protocol: Protocol.webSocket,
        useTls: true,
      ) as WebSocketEyeTrackClient;
      expect(client.useTls, isTrue);
    });

    test('all created clients start disconnected', () {
      final grpc = createClient(host: 'localhost', protocol: Protocol.grpc);
      final ws = createClient(host: 'localhost', protocol: Protocol.webSocket);
      final mqtt = createClient(
        host: 'localhost',
        protocol: Protocol.mqtt,
        clientId: 'test',
      );

      expect(grpc.connectionState, ConnectionState.disconnected);
      expect(ws.connectionState, ConnectionState.disconnected);
      expect(mqtt.connectionState, ConnectionState.disconnected);
    });
  });
}
