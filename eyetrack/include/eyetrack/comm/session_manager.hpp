#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include <eyetrack/core/error.hpp>

namespace eyetrack {

/// Protocol a client connected through.
enum class Protocol : uint8_t {
    Grpc = 0,
    WebSocket = 1,
    Mqtt = 2,
};

/// Calibration state for a client.
enum class CalibrationState : uint8_t {
    NotCalibrated = 0,
    InProgress = 1,
    Calibrated = 2,
};

/// Tracked session for a connected client.
struct ClientSession {
    std::string client_id;
    Protocol protocol = Protocol::WebSocket;
    CalibrationState calibration = CalibrationState::NotCalibrated;
};

/// Thread-safe session manager for connected clients.
class SessionManager {
public:
    SessionManager() = default;

    /// Add a client session. Returns error if client_id already exists.
    [[nodiscard]] Result<void> add(const std::string& client_id, Protocol protocol);

    /// Remove a client session. No-op if not found.
    void remove(const std::string& client_id);

    /// Get a client session. Returns nullopt if not found.
    [[nodiscard]] std::optional<ClientSession> get(
        const std::string& client_id) const;

    /// Set calibration state for a client.
    [[nodiscard]] Result<void> set_calibration(const std::string& client_id,
                                                CalibrationState state);

    /// Get number of active sessions.
    [[nodiscard]] uint32_t count() const;

    /// Check if a client exists.
    [[nodiscard]] bool has(const std::string& client_id) const;

private:
    mutable std::mutex mu_;
    std::unordered_map<std::string, ClientSession> sessions_;
};

}  // namespace eyetrack
