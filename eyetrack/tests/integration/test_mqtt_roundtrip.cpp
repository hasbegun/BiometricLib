#include <gtest/gtest.h>

#ifdef EYETRACK_HAS_MQTT

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#include <eyetrack/comm/mqtt_handler.hpp>
#include <eyetrack/core/config.hpp>

namespace {

/// Try to connect to mosquitto. Skip test if broker unavailable.
eyetrack::ServerConfig make_mqtt_config(uint16_t port = 1883) {
    eyetrack::ServerConfig config;
    // Use MQTT_HOST env var (for Docker Compose) or fallback to localhost
    const char* host = std::getenv("MQTT_HOST");  // NOLINT
    config.host = host ? host : "127.0.0.1";
    config.mqtt_port = port;
    config.tls.enabled = false;
    return config;
}

/// Helper: connect or skip the test.
bool try_connect(eyetrack::MqttHandler& handler) {
    auto result = handler.connect();
    if (!result.has_value()) {
        return false;
    }
    return true;
}

/// Synchronization helper for async message receipt.
class MessageLatch {
public:
    void set(const std::string& topic, const std::string& payload) {
        std::lock_guard<std::mutex> lock(mu_);
        topic_ = topic;
        payload_ = payload;
        received_ = true;
        cv_.notify_one();
    }

    bool wait(std::chrono::milliseconds timeout = std::chrono::milliseconds(2000)) {
        std::unique_lock<std::mutex> lock(mu_);
        return cv_.wait_for(lock, timeout, [this] { return received_; });
    }

    [[nodiscard]] std::string topic() const { return topic_; }
    [[nodiscard]] std::string payload() const { return payload_; }

private:
    std::mutex mu_;
    std::condition_variable cv_;
    bool received_ = false;
    std::string topic_;
    std::string payload_;
};

}  // namespace

TEST(MqttRoundTrip, connect_to_mosquitto) {
    auto config = make_mqtt_config();
    eyetrack::MqttHandler handler(config);

    if (!try_connect(handler)) {
        GTEST_SKIP() << "Mosquitto broker not available at 127.0.0.1:1883";
    }

    EXPECT_TRUE(handler.is_connected());
    handler.disconnect();
    EXPECT_FALSE(handler.is_connected());
}

TEST(MqttRoundTrip, publish_and_subscribe_features) {
    auto config = make_mqtt_config();

    // Subscriber
    eyetrack::MqttHandler subscriber(config);
    if (!try_connect(subscriber)) {
        GTEST_SKIP() << "Mosquitto broker not available";
    }

    MessageLatch latch;
    subscriber.set_message_handler(
        [&latch](const std::string& topic,
                 const std::string& payload) -> std::string {
            latch.set(topic, payload);
            return {};
        });

    auto sub_result = subscriber.subscribe("eyetrack/+/features", 0);
    ASSERT_TRUE(sub_result.has_value()) << sub_result.error().message;

    // Give subscription time to propagate
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Publisher
    eyetrack::MqttHandler publisher(config);
    ASSERT_TRUE(try_connect(publisher));

    auto pub_result =
        publisher.publish("eyetrack/client1/features", "test_payload", 0);
    ASSERT_TRUE(pub_result.has_value()) << pub_result.error().message;

    ASSERT_TRUE(latch.wait()) << "Timed out waiting for message";
    EXPECT_EQ(latch.topic(), "eyetrack/client1/features");
    EXPECT_EQ(latch.payload(), "test_payload");

    subscriber.disconnect();
    publisher.disconnect();
}

TEST(MqttRoundTrip, publish_gaze_event) {
    auto config = make_mqtt_config();

    eyetrack::MqttHandler subscriber(config);
    if (!try_connect(subscriber)) {
        GTEST_SKIP() << "Mosquitto broker not available";
    }

    MessageLatch latch;
    subscriber.set_message_handler(
        [&latch](const std::string& topic,
                 const std::string& payload) -> std::string {
            latch.set(topic, payload);
            return {};
        });

    auto sub_result =
        subscriber.subscribe(eyetrack::MqttHandler::gaze_topic("client42"), 1);
    ASSERT_TRUE(sub_result.has_value()) << sub_result.error().message;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    eyetrack::MqttHandler publisher(config);
    ASSERT_TRUE(try_connect(publisher));

    auto pub_result = publisher.publish(
        eyetrack::MqttHandler::gaze_topic("client42"), "gaze_data", 1);
    ASSERT_TRUE(pub_result.has_value()) << pub_result.error().message;

    ASSERT_TRUE(latch.wait()) << "Timed out waiting for gaze event";
    EXPECT_EQ(latch.topic(), "eyetrack/client42/gaze");
    EXPECT_EQ(latch.payload(), "gaze_data");

    subscriber.disconnect();
    publisher.disconnect();
}

TEST(MqttRoundTrip, protobuf_payload_roundtrip) {
    auto config = make_mqtt_config();

    eyetrack::MqttHandler subscriber(config);
    if (!try_connect(subscriber)) {
        GTEST_SKIP() << "Mosquitto broker not available";
    }

    MessageLatch latch;
    subscriber.set_message_handler(
        [&latch](const std::string& topic,
                 const std::string& payload) -> std::string {
            latch.set(topic, payload);
            return {};
        });

    (void)subscriber.subscribe("eyetrack/+/features", 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    eyetrack::MqttHandler publisher(config);
    ASSERT_TRUE(try_connect(publisher));

    // Simulated binary protobuf payload
    std::string binary_payload = {'\x08', '\x01', '\x15', '\x00',
                                  '\x00', '\x80', '\x3f', '\x1d',
                                  '\x00', '\x00', '\x00', '\x40'};
    auto pub_result =
        publisher.publish("eyetrack/proto_client/features", binary_payload, 0);
    ASSERT_TRUE(pub_result.has_value()) << pub_result.error().message;

    ASSERT_TRUE(latch.wait()) << "Timed out waiting for protobuf message";
    EXPECT_EQ(latch.payload(), binary_payload);

    subscriber.disconnect();
    publisher.disconnect();
}

TEST(MqttRoundTrip, roundtrip_latency_under_10ms) {
    auto config = make_mqtt_config();

    eyetrack::MqttHandler handler(config);
    if (!try_connect(handler)) {
        GTEST_SKIP() << "Mosquitto broker not available";
    }

    MessageLatch latch;
    handler.set_message_handler(
        [&latch](const std::string& topic,
                 const std::string& payload) -> std::string {
            latch.set(topic, payload);
            return {};
        });

    (void)handler.subscribe("eyetrack/latency/test", 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto t0 = std::chrono::steady_clock::now();
    auto pub_result = handler.publish("eyetrack/latency/test", "ping", 0);
    ASSERT_TRUE(pub_result.has_value()) << pub_result.error().message;

    ASSERT_TRUE(latch.wait(std::chrono::milliseconds(100)))
        << "Timed out waiting for latency message";

    auto t1 = std::chrono::steady_clock::now();
    auto latency_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    EXPECT_LT(latency_ms, 10) << "Roundtrip latency: " << latency_ms << "ms";

    handler.disconnect();
}

TEST(MqttRoundTrip, disconnect_and_reconnect) {
    auto config = make_mqtt_config();

    eyetrack::MqttHandler handler(config);
    if (!try_connect(handler)) {
        GTEST_SKIP() << "Mosquitto broker not available";
    }

    EXPECT_TRUE(handler.is_connected());
    handler.disconnect();
    EXPECT_FALSE(handler.is_connected());

    // Reconnect
    auto result = handler.connect();
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_TRUE(handler.is_connected());

    handler.disconnect();
}

TEST(MqttRoundTrip, multiple_clients_different_ids) {
    auto config = make_mqtt_config();

    eyetrack::MqttHandler sub1(config);
    if (!try_connect(sub1)) {
        GTEST_SKIP() << "Mosquitto broker not available";
    }

    eyetrack::MqttHandler sub2(config);
    ASSERT_TRUE(try_connect(sub2));

    MessageLatch latch1;
    MessageLatch latch2;

    sub1.set_message_handler(
        [&latch1](const std::string& topic,
                  const std::string& payload) -> std::string {
            latch1.set(topic, payload);
            return {};
        });

    sub2.set_message_handler(
        [&latch2](const std::string& topic,
                  const std::string& payload) -> std::string {
            latch2.set(topic, payload);
            return {};
        });

    (void)sub1.subscribe(eyetrack::MqttHandler::gaze_topic("user_a"), 1);
    (void)sub2.subscribe(eyetrack::MqttHandler::gaze_topic("user_b"), 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    eyetrack::MqttHandler publisher(config);
    ASSERT_TRUE(try_connect(publisher));

    (void)publisher.publish(eyetrack::MqttHandler::gaze_topic("user_a"),
                            "gaze_a", 1);
    (void)publisher.publish(eyetrack::MqttHandler::gaze_topic("user_b"),
                            "gaze_b", 1);

    ASSERT_TRUE(latch1.wait()) << "Timed out waiting for user_a message";
    ASSERT_TRUE(latch2.wait()) << "Timed out waiting for user_b message";

    EXPECT_EQ(latch1.payload(), "gaze_a");
    EXPECT_EQ(latch2.payload(), "gaze_b");

    sub1.disconnect();
    sub2.disconnect();
    publisher.disconnect();
}

#else

TEST(MqttRoundTrip, skipped_without_mqtt) {
    GTEST_SKIP() << "MQTT support not enabled";
}

#endif  // EYETRACK_HAS_MQTT
