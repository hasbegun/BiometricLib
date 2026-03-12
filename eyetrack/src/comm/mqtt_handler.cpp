#ifdef EYETRACK_HAS_MQTT

#include <eyetrack/comm/mqtt_handler.hpp>

#include <chrono>
#include <filesystem>
#include <utility>

#include <mqtt/async_client.h>
#include <spdlog/spdlog.h>

namespace eyetrack {

/// Pimpl to isolate Paho headers from the public API.
struct MqttHandler::Impl : public mqtt::callback {
    mqtt::async_client client;
    MqttMessageHandler* handler_ptr = nullptr;

    explicit Impl(const std::string& uri, const std::string& client_id)
        : client(uri, client_id) {}

    // mqtt::callback overrides
    void message_arrived(mqtt::const_message_ptr msg) override {
        if (handler_ptr && *handler_ptr) {
            (*handler_ptr)(msg->get_topic(), msg->get_payload_str());
        }
    }

    void connection_lost(const std::string& cause) override {
        spdlog::warn("MQTT connection lost: {}", cause.empty() ? "unknown" : cause);
    }
};

MqttHandler::MqttHandler(const ServerConfig& config) : config_(config) {}

MqttHandler::~MqttHandler() {
    disconnect();
}

void MqttHandler::set_message_handler(MqttMessageHandler handler) {
    handler_ = std::move(handler);
}

std::string MqttHandler::broker_uri() const {
    std::string scheme = config_.tls.enabled ? "ssl" : "tcp";
    return scheme + "://" + config_.host + ":" + std::to_string(config_.mqtt_port);
}

Result<void> MqttHandler::connect() {
    if (connected_.load(std::memory_order_acquire)) {
        return make_error(ErrorCode::ConfigInvalid, "MQTT handler already connected");
    }

    // Validate TLS config if enabled
    if (config_.tls.enabled) {
        if (!std::filesystem::exists(config_.tls.cert_path)) {
            return make_error(ErrorCode::ConfigInvalid,
                              "TLS cert file not found: " + config_.tls.cert_path);
        }
        if (!std::filesystem::exists(config_.tls.key_path)) {
            return make_error(ErrorCode::ConfigInvalid,
                              "TLS key file not found: " + config_.tls.key_path);
        }
    }

    auto uri = broker_uri();
    static std::atomic<uint32_t> id_counter{0};
    auto client_id =
        "eyetrack_" + std::to_string(id_counter.fetch_add(1, std::memory_order_relaxed));
    impl_ = std::make_unique<Impl>(uri, client_id);
    impl_->handler_ptr = &handler_;
    impl_->client.set_callback(*impl_);

    mqtt::connect_options conn_opts;
    conn_opts.set_clean_session(true);
    conn_opts.set_keep_alive_interval(std::chrono::seconds(20));
    conn_opts.set_automatic_reconnect(true);

    if (config_.tls.enabled) {
        mqtt::ssl_options ssl_opts;
        ssl_opts.set_trust_store(config_.tls.ca_path.empty()
                                     ? config_.tls.cert_path
                                     : config_.tls.ca_path);
        ssl_opts.set_key_store(config_.tls.cert_path);
        ssl_opts.set_private_key(config_.tls.key_path);
        conn_opts.set_ssl(ssl_opts);
    }

    try {
        auto tok = impl_->client.connect(conn_opts);
        tok->wait_for(std::chrono::seconds(5));
        if (!impl_->client.is_connected()) {
            impl_.reset();
            return make_error(ErrorCode::ConnectionFailed,
                              "Failed to connect to MQTT broker at " + uri);
        }
    } catch (const mqtt::exception& e) {
        impl_.reset();
        return make_error(ErrorCode::ConnectionFailed,
                          std::string("MQTT connection error: ") + e.what());
    }

    connected_.store(true, std::memory_order_release);
    spdlog::info("MQTT connected to {}", uri);
    return {};
}

void MqttHandler::disconnect() {
    if (connected_.exchange(false, std::memory_order_acq_rel)) {
        if (impl_ && impl_->client.is_connected()) {
            try {
                impl_->client.disconnect()->wait_for(std::chrono::seconds(2));
            } catch (const mqtt::exception& e) {
                spdlog::warn("MQTT disconnect error: {}", e.what());
            }
        }
        impl_.reset();
        spdlog::info("MQTT disconnected");
    }
}

bool MqttHandler::is_connected() const noexcept {
    return connected_.load(std::memory_order_acquire);
}

Result<void> MqttHandler::publish(const std::string& topic,
                                   const std::string& payload, int qos) {
    if (!connected_.load(std::memory_order_acquire) || !impl_) {
        return make_error(ErrorCode::ConnectionFailed, "MQTT not connected");
    }

    try {
        auto msg = mqtt::make_message(topic, payload, qos, false);
        impl_->client.publish(msg)->wait_for(std::chrono::seconds(5));
    } catch (const mqtt::exception& e) {
        return make_error(ErrorCode::ConnectionFailed,
                          std::string("MQTT publish error: ") + e.what());
    }
    return {};
}

Result<void> MqttHandler::subscribe(const std::string& topic_filter, int qos) {
    if (!connected_.load(std::memory_order_acquire) || !impl_) {
        return make_error(ErrorCode::ConnectionFailed, "MQTT not connected");
    }

    try {
        impl_->client.subscribe(topic_filter, qos)->wait_for(
            std::chrono::seconds(5));
    } catch (const mqtt::exception& e) {
        return make_error(ErrorCode::ConnectionFailed,
                          std::string("MQTT subscribe error: ") + e.what());
    }

    spdlog::info("MQTT subscribed to '{}' (QoS {})", topic_filter, qos);
    return {};
}

}  // namespace eyetrack

#endif  // EYETRACK_HAS_MQTT
