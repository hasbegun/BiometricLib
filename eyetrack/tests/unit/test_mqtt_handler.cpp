#include <gtest/gtest.h>

#ifdef EYETRACK_HAS_MQTT

#include <eyetrack/comm/mqtt_handler.hpp>
#include <eyetrack/core/config.hpp>

TEST(MqttHandler, constructs_with_config) {
    eyetrack::ServerConfig config;
    config.mqtt_port = 1883;
    config.tls.enabled = false;

    eyetrack::MqttHandler handler(config);
    EXPECT_FALSE(handler.is_connected());
    EXPECT_EQ(handler.port(), 1883);
}

TEST(MqttHandler, topic_format_correct) {
    EXPECT_EQ(eyetrack::MqttHandler::features_topic(), "eyetrack/+/features");
    EXPECT_EQ(eyetrack::MqttHandler::gaze_topic("client42"),
              "eyetrack/client42/gaze");
    EXPECT_EQ(eyetrack::MqttHandler::gaze_topic("abc_123"),
              "eyetrack/abc_123/gaze");
}

TEST(MqttHandler, qos_settings) {
    EXPECT_EQ(eyetrack::MqttHandler::features_qos(), 0);
    EXPECT_EQ(eyetrack::MqttHandler::gaze_qos(), 1);
}

TEST(MqttHandler, tls_disabled_uses_plain_tcp) {
    eyetrack::ServerConfig config;
    config.mqtt_port = 1883;
    config.tls.enabled = false;

    eyetrack::MqttHandler handler(config);
    EXPECT_FALSE(handler.tls_enabled());
}

TEST(MqttHandler, tls_enabled_configures_ssl) {
    eyetrack::ServerConfig config;
    config.mqtt_port = 8883;
    config.tls.enabled = true;
    config.tls.cert_path = "/nonexistent/cert.pem";
    config.tls.key_path = "/nonexistent/key.pem";

    eyetrack::MqttHandler handler(config);
    EXPECT_TRUE(handler.tls_enabled());

    // connect should fail with invalid cert path
    auto result = handler.connect();
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::ConfigInvalid);
}

#else

TEST(MqttHandler, skipped_without_mqtt) {
    GTEST_SKIP() << "MQTT support not enabled";
}

#endif  // EYETRACK_HAS_MQTT
