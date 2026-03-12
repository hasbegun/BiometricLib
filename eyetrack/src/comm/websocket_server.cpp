#ifdef EYETRACK_HAS_WEBSOCKET

#include <eyetrack/comm/websocket_server.hpp>

#include <filesystem>
#include <utility>

#include <spdlog/spdlog.h>

namespace eyetrack {

namespace {

/// A single WebSocket session, kept alive via shared_ptr/enable_shared_from_this.
class WsSession : public std::enable_shared_from_this<WsSession> {
public:
    WsSession(tcp::socket socket, std::atomic<uint32_t>& client_count,
              WsMessageHandler& handler, std::atomic<bool>& running)
        : ws_(std::move(socket)),
          client_count_(client_count),
          handler_(handler),
          running_(running) {
        boost::system::error_code ec;
        auto ep = ws_.next_layer().remote_endpoint(ec);
        if (!ec) {
            client_id_ = ep.address().to_string() + ":" + std::to_string(ep.port());
        } else {
            client_id_ = "unknown";
        }
    }

    void run() {
        client_count_.fetch_add(1, std::memory_order_relaxed);
        ws_.binary(true);

        auto self = shared_from_this();
        ws_.async_accept([self](boost::system::error_code ec) {
            if (ec) {
                spdlog::warn("WebSocket handshake failed for {}: {}",
                             self->client_id_, ec.message());
                self->client_count_.fetch_sub(1, std::memory_order_relaxed);
                return;
            }
            spdlog::debug("Client connected: {}", self->client_id_);
            self->do_read();
        });
    }

private:
    void do_read() {
        auto self = shared_from_this();
        ws_.async_read(buf_, [self](boost::system::error_code ec,
                                    std::size_t /*bytes_transferred*/) {
            if (ec) {
                if (ec != beast::websocket::error::closed) {
                    spdlog::debug("Client {} read error: {}",
                                  self->client_id_, ec.message());
                }
                self->client_count_.fetch_sub(1, std::memory_order_relaxed);
                return;
            }

            if (self->handler_) {
                auto data = beast::buffers_to_string(self->buf_.data());
                auto response = self->handler_(data, self->client_id_);
                self->buf_.consume(self->buf_.size());

                if (!response.empty()) {
                    auto resp_buf =
                        std::make_shared<std::string>(std::move(response));
                    self->ws_.async_write(
                        net::buffer(*resp_buf),
                        [self, resp_buf](boost::system::error_code ec,
                                         std::size_t /*bytes_transferred*/) {
                            if (ec) {
                                self->client_count_.fetch_sub(
                                    1, std::memory_order_relaxed);
                                return;
                            }
                            if (self->running_.load(std::memory_order_acquire)) {
                                self->do_read();
                            }
                        });
                    return;
                }
            } else {
                self->buf_.consume(self->buf_.size());
            }

            if (self->running_.load(std::memory_order_acquire)) {
                self->do_read();
            }
        });
    }

    beast::websocket::stream<tcp::socket> ws_;
    beast::flat_buffer buf_;
    std::string client_id_;
    std::atomic<uint32_t>& client_count_;
    WsMessageHandler& handler_;
    std::atomic<bool>& running_;
};

}  // namespace

WebSocketServer::WebSocketServer(const ServerConfig& config) : config_(config) {}

WebSocketServer::~WebSocketServer() {
    stop();
}

void WebSocketServer::set_message_handler(WsMessageHandler handler) {
    handler_ = std::move(handler);
}

Result<void> WebSocketServer::start() {
    if (running_.load(std::memory_order_acquire)) {
        return make_error(ErrorCode::ConfigInvalid, "WebSocket server already running");
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

    boost::system::error_code ec;
    auto endpoint = tcp::endpoint(net::ip::make_address(config_.host, ec),
                                  config_.websocket_port);
    if (ec) {
        return make_error(ErrorCode::ConnectionFailed,
                          "Invalid host address: " + ec.message());
    }

    acceptor_ = std::make_unique<tcp::acceptor>(ioc_);
    acceptor_->open(endpoint.protocol(), ec);
    if (ec) {
        return make_error(ErrorCode::ConnectionFailed,
                          "Failed to open acceptor: " + ec.message());
    }

    acceptor_->set_option(net::socket_base::reuse_address(true), ec);
    acceptor_->bind(endpoint, ec);
    if (ec) {
        return make_error(ErrorCode::ConnectionFailed,
                          "Failed to bind port " +
                              std::to_string(config_.websocket_port) + ": " + ec.message());
    }

    acceptor_->listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        return make_error(ErrorCode::ConnectionFailed,
                          "Failed to listen: " + ec.message());
    }

    running_.store(true, std::memory_order_release);
    do_accept();

    io_thread_ = std::thread([this] {
        ioc_.run();
    });

    spdlog::info("WebSocket server started on {}:{}", config_.host,
                 config_.websocket_port);
    return {};
}

void WebSocketServer::stop() {
    if (running_.exchange(false, std::memory_order_acq_rel)) {
        boost::system::error_code ec;
        if (acceptor_ && acceptor_->is_open()) {
            acceptor_->close(ec);
        }
        ioc_.stop();
        if (io_thread_.joinable()) {
            io_thread_.join();
        }
        spdlog::info("WebSocket server stopped");
    }
}

bool WebSocketServer::is_running() const noexcept {
    return running_.load(std::memory_order_acquire);
}

uint32_t WebSocketServer::connected_clients() const noexcept {
    return client_count_.load(std::memory_order_relaxed);
}

void WebSocketServer::do_accept() {
    acceptor_->async_accept([this](boost::system::error_code ec, tcp::socket socket) {
        if (ec) {
            if (running_.load(std::memory_order_acquire)) {
                spdlog::warn("Accept error: {}", ec.message());
            }
            return;
        }
        on_session(std::move(socket));
        if (running_.load(std::memory_order_acquire)) {
            do_accept();
        }
    });
}

void WebSocketServer::on_session(tcp::socket socket) {
    std::make_shared<WsSession>(std::move(socket), client_count_, handler_,
                                running_)
        ->run();
}

}  // namespace eyetrack

#endif  // EYETRACK_HAS_WEBSOCKET
