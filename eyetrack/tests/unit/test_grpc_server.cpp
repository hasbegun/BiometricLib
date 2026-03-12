#include <gtest/gtest.h>

#ifdef EYETRACK_HAS_GRPC

#include <filesystem>
#include <fstream>

#include <eyetrack/comm/grpc_server.hpp>
#include <eyetrack/core/config.hpp>

namespace fs = std::filesystem;

namespace {

const std::string kProjectRoot = EYETRACK_PROJECT_ROOT;

// Helper to create a self-signed cert/key pair for testing.
// Returns (cert_path, key_path) or empty strings on failure.
struct TlsFixture {
    std::string cert_path;
    std::string key_path;
    std::string dir;

    TlsFixture() {
        dir = kProjectRoot + "/tests/fixtures/tls_test_" +
              std::to_string(::testing::UnitTest::GetInstance()
                                 ->current_test_info()
                                 ->line());
        fs::create_directories(dir);
        cert_path = dir + "/server.crt";
        key_path = dir + "/server.key";

        // Generate self-signed cert using openssl
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

TEST(GrpcServer, starts_on_configured_port) {
    eyetrack::ServerConfig config;
    config.grpc_port = 50100;
    config.tls.enabled = false;

    eyetrack::GrpcServer server(config);
    auto result = server.start();
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_TRUE(server.is_running());
    EXPECT_EQ(server.port(), 50100);

    server.stop();
    EXPECT_FALSE(server.is_running());
}

TEST(GrpcServer, stops_gracefully) {
    eyetrack::ServerConfig config;
    config.grpc_port = 50101;
    config.tls.enabled = false;

    eyetrack::GrpcServer server(config);
    auto result = server.start();
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_TRUE(server.is_running());

    // Stop should be clean
    server.stop();
    EXPECT_FALSE(server.is_running());

    // Double stop should be safe
    server.stop();
    EXPECT_FALSE(server.is_running());
}

TEST(GrpcServer, tls_disabled_uses_insecure_credentials) {
    eyetrack::ServerConfig config;
    config.grpc_port = 50102;
    config.tls.enabled = false;

    eyetrack::GrpcServer server(config);
    EXPECT_FALSE(server.tls_enabled());

    auto result = server.start();
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_TRUE(server.is_running());

    server.stop();
}

TEST(GrpcServer, tls_enabled_uses_ssl_credentials) {
    TlsFixture tls;
    if (!tls.valid()) {
        GTEST_SKIP() << "Could not generate self-signed certs (openssl not available)";
    }

    eyetrack::ServerConfig config;
    config.grpc_port = 50103;
    config.tls.enabled = true;
    config.tls.cert_path = tls.cert_path;
    config.tls.key_path = tls.key_path;

    eyetrack::GrpcServer server(config);
    EXPECT_TRUE(server.tls_enabled());

    auto result = server.start();
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_TRUE(server.is_running());

    server.stop();
}

TEST(GrpcServer, invalid_tls_cert_returns_error) {
    eyetrack::ServerConfig config;
    config.grpc_port = 50104;
    config.tls.enabled = true;
    config.tls.cert_path = "/nonexistent/cert.pem";
    config.tls.key_path = "/nonexistent/key.pem";

    eyetrack::GrpcServer server(config);
    auto result = server.start();
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, eyetrack::ErrorCode::ConfigInvalid);
}

#else

TEST(GrpcServer, skipped_without_grpc) {
    GTEST_SKIP() << "gRPC support not enabled";
}

#endif  // EYETRACK_HAS_GRPC
