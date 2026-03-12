#include <gtest/gtest.h>

#ifdef EYETRACK_HAS_WEBSOCKET

#include <filesystem>
#include <fstream>

#include <eyetrack/comm/websocket_server.hpp>
#include <eyetrack/core/config.hpp>

namespace fs = std::filesystem;

namespace {

const std::string kProjectRoot = EYETRACK_PROJECT_ROOT;

struct TlsFixture {
    std::string cert_path;
    std::string key_path;
    std::string dir;

    TlsFixture() {
        dir = kProjectRoot + "/tests/fixtures/ws_tls_test_" +
              std::to_string(::testing::UnitTest::GetInstance()
                                 ->current_test_info()
                                 ->line());
        fs::create_directories(dir);
        cert_path = dir + "/server.crt";
        key_path = dir + "/server.key";

        std::string cmd =
            "openssl req -x509 -newkey rsa:2048 -keyout " + key_path +
            " -out " + cert_path +
            " -days 1 -nodes -subj '/CN=localhost' 2>/dev/null";
        (void)std::system(cmd.c_str());  // NOLINT
    }

    ~TlsFixture() {
        fs::remove_all(dir);
    }

    [[nodiscard]] bool valid() const {
        return fs::exists(cert_path) && fs::exists(key_path);
    }
};

}  // namespace

TEST(WebSocketServer, starts_on_configured_port) {
    eyetrack::ServerConfig config;
    config.websocket_port = 9100;
    config.tls.enabled = false;

    eyetrack::WebSocketServer server(config);
    auto result = server.start();
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_TRUE(server.is_running());
    EXPECT_EQ(server.port(), 9100);

    server.stop();
    EXPECT_FALSE(server.is_running());
}

TEST(WebSocketServer, stops_gracefully) {
    eyetrack::ServerConfig config;
    config.websocket_port = 9101;
    config.tls.enabled = false;

    eyetrack::WebSocketServer server(config);
    auto result = server.start();
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_TRUE(server.is_running());

    server.stop();
    EXPECT_FALSE(server.is_running());

    // Double stop should be safe
    server.stop();
    EXPECT_FALSE(server.is_running());
}

TEST(WebSocketServer, tls_disabled_starts_plain) {
    eyetrack::ServerConfig config;
    config.websocket_port = 9102;
    config.tls.enabled = false;

    eyetrack::WebSocketServer server(config);
    EXPECT_FALSE(server.tls_enabled());

    auto result = server.start();
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_TRUE(server.is_running());

    server.stop();
}

TEST(WebSocketServer, invalid_tls_cert_returns_error) {
    eyetrack::ServerConfig config;
    config.websocket_port = 9103;
    config.tls.enabled = true;
    config.tls.cert_path = "/nonexistent/cert.pem";
    config.tls.key_path = "/nonexistent/key.pem";

    eyetrack::WebSocketServer server(config);
    auto result = server.start();
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::ConfigInvalid);
}

TEST(WebSocketServer, reports_zero_clients_initially) {
    eyetrack::ServerConfig config;
    config.websocket_port = 9104;
    config.tls.enabled = false;

    eyetrack::WebSocketServer server(config);
    EXPECT_EQ(server.connected_clients(), 0U);

    auto result = server.start();
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(server.connected_clients(), 0U);

    server.stop();
}

#else

TEST(WebSocketServer, skipped_without_websocket) {
    GTEST_SKIP() << "WebSocket support not enabled";
}

#endif  // EYETRACK_HAS_WEBSOCKET
