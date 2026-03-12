#pragma once

#include <atomic>
#include <memory>

#include <eyetrack/comm/gateway.hpp>
#include <eyetrack/core/config.hpp>
#include <eyetrack/core/error.hpp>

#ifdef EYETRACK_HAS_GRPC
#include <eyetrack/comm/grpc_server.hpp>
#endif

#ifdef EYETRACK_HAS_WEBSOCKET
#include <eyetrack/comm/websocket_server.hpp>
#endif

#ifdef EYETRACK_HAS_MQTT
#include <eyetrack/comm/mqtt_handler.hpp>
#endif

namespace eyetrack {

/// Top-level server that starts all protocol handlers and the gateway.
class EyetrackServer {
public:
    explicit EyetrackServer(const ServerConfig& config);
    ~EyetrackServer();

    EyetrackServer(const EyetrackServer&) = delete;
    EyetrackServer& operator=(const EyetrackServer&) = delete;

    /// Set the pipeline dispatcher on the gateway.
    void set_dispatcher(PipelineDispatcher dispatcher);

    /// Start all protocol servers.
    [[nodiscard]] Result<void> start();

    /// Stop all protocol servers gracefully.
    void stop();

    /// Check if running.
    [[nodiscard]] bool is_running() const noexcept;

    /// Access the gateway.
    [[nodiscard]] Gateway& gateway() noexcept { return gateway_; }

private:
    ServerConfig config_;
    Gateway gateway_;
    std::atomic<bool> running_{false};

#ifdef EYETRACK_HAS_GRPC
    std::unique_ptr<GrpcServer> grpc_;
#endif
#ifdef EYETRACK_HAS_WEBSOCKET
    std::unique_ptr<WebSocketServer> websocket_;
#endif
#ifdef EYETRACK_HAS_MQTT
    std::unique_ptr<MqttHandler> mqtt_;
#endif
};

}  // namespace eyetrack
