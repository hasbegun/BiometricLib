#pragma once

#ifdef EYETRACK_HAS_MQTT

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include <eyetrack/core/config.hpp>
#include <eyetrack/core/error.hpp>

namespace eyetrack {

/// Callback for incoming MQTT messages.
/// Receives topic and payload, returns optional response payload.
using MqttMessageHandler =
    std::function<std::string(const std::string& topic, const std::string& payload)>;

/// MQTT handler using Eclipse Paho C++ async client.
///
/// Subscribes to `eyetrack/+/features` (QoS 0) and publishes to
/// `eyetrack/{client_id}/gaze` (QoS 1).
class MqttHandler {
public:
    explicit MqttHandler(const ServerConfig& config);
    ~MqttHandler();

    MqttHandler(const MqttHandler&) = delete;
    MqttHandler& operator=(const MqttHandler&) = delete;

    /// Set the handler for incoming messages.
    void set_message_handler(MqttMessageHandler handler);

    /// Connect to the MQTT broker. Returns error if connection fails.
    [[nodiscard]] Result<void> connect();

    /// Disconnect from the broker gracefully.
    void disconnect();

    /// Check if connected.
    [[nodiscard]] bool is_connected() const noexcept;

    /// Publish a message to a topic.
    [[nodiscard]] Result<void> publish(const std::string& topic,
                                       const std::string& payload,
                                       int qos = 1);

    /// Subscribe to a topic pattern.
    [[nodiscard]] Result<void> subscribe(const std::string& topic_filter,
                                          int qos = 0);

    /// Get the features subscribe topic pattern.
    [[nodiscard]] static std::string features_topic() {
        return "eyetrack/+/features";
    }

    /// Get the gaze publish topic for a specific client.
    [[nodiscard]] static std::string gaze_topic(const std::string& client_id) {
        return "eyetrack/" + client_id + "/gaze";
    }

    /// Get the port.
    [[nodiscard]] uint16_t port() const noexcept {
        return config_.mqtt_port;
    }

    /// Check if TLS is enabled.
    [[nodiscard]] bool tls_enabled() const noexcept {
        return config_.tls.enabled;
    }

    /// Get features QoS level.
    [[nodiscard]] static int features_qos() noexcept { return 0; }

    /// Get gaze QoS level.
    [[nodiscard]] static int gaze_qos() noexcept { return 1; }

private:
    ServerConfig config_;
    MqttMessageHandler handler_;
    std::atomic<bool> connected_{false};

    struct Impl;
    std::unique_ptr<Impl> impl_;

    std::string broker_uri() const;
};

}  // namespace eyetrack

#endif  // EYETRACK_HAS_MQTT
