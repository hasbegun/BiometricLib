#pragma once

#ifdef EYETRACK_HAS_WEBSOCKET

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <eyetrack/core/config.hpp>
#include <eyetrack/core/error.hpp>

namespace eyetrack {

namespace beast = boost::beast;
namespace net = boost::asio;
using tcp = net::ip::tcp;

/// Callback for incoming binary messages.
using WsMessageHandler =
    std::function<std::string(const std::string& data, const std::string& client_id)>;

/// WebSocket server using Boost.Beast.
///
/// Supports plain WS and WSS (TLS). Binary frames with protobuf payloads.
/// Manages connections with heartbeat (ping/pong) and clean disconnect.
class WebSocketServer {
public:
    explicit WebSocketServer(const ServerConfig& config);
    ~WebSocketServer();

    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;

    /// Set the handler for incoming messages.
    /// Handler receives raw bytes + client_id, returns response bytes.
    void set_message_handler(WsMessageHandler handler);

    /// Start the server. Returns error if port binding fails.
    [[nodiscard]] Result<void> start();

    /// Stop the server gracefully.
    void stop();

    /// Check if running.
    [[nodiscard]] bool is_running() const noexcept;

    /// Get the port.
    [[nodiscard]] uint16_t port() const noexcept {
        return config_.websocket_port;
    }

    /// Get connected client count.
    [[nodiscard]] uint32_t connected_clients() const noexcept;

    /// Check if TLS is enabled.
    [[nodiscard]] bool tls_enabled() const noexcept {
        return config_.tls.enabled;
    }

private:
    ServerConfig config_;
    net::io_context ioc_{1};
    std::unique_ptr<tcp::acceptor> acceptor_;
    std::thread io_thread_;
    std::atomic<bool> running_{false};
    std::atomic<uint32_t> client_count_{0};
    WsMessageHandler handler_;

    void do_accept();
    void on_session(tcp::socket socket);
};

}  // namespace eyetrack

#endif  // EYETRACK_HAS_WEBSOCKET
