#include <eyetrack/comm/eyetrack_server.hpp>

#include <utility>

#include <spdlog/spdlog.h>

namespace eyetrack {

EyetrackServer::EyetrackServer(const ServerConfig& config) : config_(config) {}

EyetrackServer::~EyetrackServer() {
    stop();
}

void EyetrackServer::set_dispatcher(PipelineDispatcher dispatcher) {
    gateway_.set_dispatcher(std::move(dispatcher));
}

Result<void> EyetrackServer::start() {
    if (running_.load(std::memory_order_acquire)) {
        return make_error(ErrorCode::ConfigInvalid, "Server already running");
    }

#ifdef EYETRACK_HAS_GRPC
    grpc_ = std::make_unique<GrpcServer>(config_);
    auto grpc_result = grpc_->start();
    if (!grpc_result.has_value()) {
        spdlog::error("Failed to start gRPC server: {}",
                      grpc_result.error().message);
        return std::unexpected(grpc_result.error());
    }
    spdlog::info("gRPC server listening on port {}", config_.grpc_port);
#endif

#ifdef EYETRACK_HAS_WEBSOCKET
    websocket_ = std::make_unique<WebSocketServer>(config_);
    auto ws_result = websocket_->start();
    if (!ws_result.has_value()) {
        spdlog::error("Failed to start WebSocket server: {}",
                      ws_result.error().message);
#ifdef EYETRACK_HAS_GRPC
        if (grpc_) grpc_->stop();
#endif
        return std::unexpected(ws_result.error());
    }
    spdlog::info("WebSocket server listening on port {}",
                 config_.websocket_port);
#endif

#ifdef EYETRACK_HAS_MQTT
    mqtt_ = std::make_unique<MqttHandler>(config_);
    auto mqtt_result = mqtt_->connect();
    if (!mqtt_result.has_value()) {
        spdlog::warn("MQTT connection failed (broker may not be running): {}",
                     mqtt_result.error().message);
        // MQTT failure is non-fatal — broker might not be available
        mqtt_.reset();
    } else {
        // Subscribe to features topic
        (void)mqtt_->subscribe(MqttHandler::features_topic(),
                               MqttHandler::features_qos());
        spdlog::info("MQTT connected on port {}", config_.mqtt_port);
    }
#endif

    running_.store(true, std::memory_order_release);
    spdlog::info("EyeTrack server started");
    return {};
}

void EyetrackServer::stop() {
    if (running_.exchange(false, std::memory_order_acq_rel)) {
#ifdef EYETRACK_HAS_MQTT
        if (mqtt_) {
            mqtt_->disconnect();
            mqtt_.reset();
        }
#endif
#ifdef EYETRACK_HAS_WEBSOCKET
        if (websocket_) {
            websocket_->stop();
            websocket_.reset();
        }
#endif
#ifdef EYETRACK_HAS_GRPC
        if (grpc_) {
            grpc_->stop();
            grpc_.reset();
        }
#endif
        spdlog::info("EyeTrack server stopped");
    }
}

bool EyetrackServer::is_running() const noexcept {
    return running_.load(std::memory_order_acquire);
}

}  // namespace eyetrack
