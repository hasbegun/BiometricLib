#pragma once

#ifdef EYETRACK_HAS_GRPC

#include <atomic>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include <eyetrack/core/config.hpp>
#include <eyetrack/core/error.hpp>

namespace eyetrack {

/// gRPC server for the EyeTrack service.
///
/// Implements StreamGaze, ProcessFrame, Calibrate, and GetStatus RPCs.
/// Supports TLS when configured via ServerConfig.
class GrpcServer {
public:
    explicit GrpcServer(const ServerConfig& config);
    ~GrpcServer();

    GrpcServer(const GrpcServer&) = delete;
    GrpcServer& operator=(const GrpcServer&) = delete;

    /// Start the server. Returns error if port binding fails or TLS certs invalid.
    [[nodiscard]] Result<void> start();

    /// Stop the server gracefully.
    void stop();

    /// Check if the server is running.
    [[nodiscard]] bool is_running() const noexcept;

    /// Get the port the server is listening on.
    [[nodiscard]] uint16_t port() const noexcept { return config_.grpc_port; }

    /// Get the number of connected clients (approximate).
    [[nodiscard]] uint32_t connected_clients() const noexcept;

    /// Check if TLS is enabled.
    [[nodiscard]] bool tls_enabled() const noexcept {
        return config_.tls.enabled;
    }

private:
    ServerConfig config_;
    std::unique_ptr<grpc::Server> server_;
    std::atomic<bool> running_{false};
    std::atomic<uint32_t> client_count_{0};

    /// Build server credentials based on TLS config.
    [[nodiscard]] Result<std::shared_ptr<grpc::ServerCredentials>>
    build_credentials() const;
};

}  // namespace eyetrack

#endif  // EYETRACK_HAS_GRPC
