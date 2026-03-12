#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

#include <eyetrack/comm/eyetrack_server.hpp>
#include <eyetrack/core/config.hpp>

namespace fs = std::filesystem;

namespace {

const std::string kProjectRoot = EYETRACK_PROJECT_ROOT;

eyetrack::ServerConfig make_test_config() {
    eyetrack::ServerConfig config;
    config.host = "0.0.0.0";
    config.grpc_port = 50300;
    config.websocket_port = 9300;
    config.mqtt_port = 1883;
    config.tls.enabled = false;
    return config;
}

eyetrack::PipelineDispatcher make_test_dispatcher() {
    return [](const eyetrack::FrameData& /*frame*/)
               -> eyetrack::Result<eyetrack::GazeResult> {
        eyetrack::GazeResult gaze;
        gaze.gaze_point = {0.5F, 0.5F};
        gaze.confidence = 1.0F;
        gaze.blink = eyetrack::BlinkType::None;
        return gaze;
    };
}

}  // namespace

TEST(ServerStartup, starts_all_protocols) {
    auto config = make_test_config();
    eyetrack::EyetrackServer server(config);
    server.set_dispatcher(make_test_dispatcher());

    auto result = server.start();
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_TRUE(server.is_running());

    server.stop();
    EXPECT_FALSE(server.is_running());
}

TEST(ServerStartup, reads_server_yaml) {
    auto config_path = fs::path(kProjectRoot) / "config" / "server.yaml";
    if (!fs::exists(config_path)) {
        GTEST_SKIP() << "config/server.yaml not found";
    }

    auto result = eyetrack::load_server_config(config_path);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    auto config = result.value();
    EXPECT_EQ(config.grpc_port, 50051);
    EXPECT_EQ(config.websocket_port, 8080);
    EXPECT_EQ(config.mqtt_port, 1883);
    EXPECT_FALSE(config.tls.enabled);
}

TEST(ServerStartup, graceful_shutdown_on_stop) {
    auto config = make_test_config();
    config.grpc_port = 50301;
    config.websocket_port = 9301;

    eyetrack::EyetrackServer server(config);
    server.set_dispatcher(make_test_dispatcher());

    auto result = server.start();
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_TRUE(server.is_running());

    // Stop from another thread (simulates signal handler)
    std::thread stopper([&server] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        server.stop();
    });

    stopper.join();
    EXPECT_FALSE(server.is_running());

    // Double stop should be safe
    server.stop();
    EXPECT_FALSE(server.is_running());
}

TEST(ServerStartup, rejects_invalid_input) {
    auto config = make_test_config();
    config.grpc_port = 50302;
    config.websocket_port = 9302;

    eyetrack::EyetrackServer server(config);
    server.set_dispatcher(make_test_dispatcher());

    auto result = server.start();
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Send invalid input through the gateway
    eyetrack::IncomingMessage msg;
    msg.client_id = "invalid@client!";  // Invalid chars
    msg.auth_token = "test";
    msg.frame.frame = cv::Mat(480, 640, CV_8UC3);

    auto gw_result = server.gateway().process(msg);
    EXPECT_FALSE(gw_result.has_value());
    EXPECT_EQ(gw_result.error().code, eyetrack::ErrorCode::InvalidInput);

    server.stop();
}

TEST(ServerStartup, rate_limiter_active) {
    auto config = make_test_config();
    config.grpc_port = 50303;
    config.websocket_port = 9303;

    eyetrack::EyetrackServer server(config);
    server.set_dispatcher(make_test_dispatcher());
    server.gateway().set_rate_limit(10);  // Low limit for testing

    auto result = server.start();
    ASSERT_TRUE(result.has_value()) << result.error().message;

    eyetrack::IncomingMessage msg;
    msg.client_id = "rate_test_client";
    msg.auth_token = "rate_test_client";
    msg.protocol = eyetrack::Protocol::WebSocket;
    msg.frame.frame = cv::Mat(480, 640, CV_8UC3, cv::Scalar(128, 128, 128));
    msg.frame.frame_id = 1;
    msg.frame.timestamp = std::chrono::steady_clock::now();
    msg.frame.client_id = "rate_test_client";

    // Exhaust rate limit
    int allowed = 0;
    for (int i = 0; i < 15; ++i) {
        msg.frame.timestamp = std::chrono::steady_clock::now();
        auto gw_result = server.gateway().process(msg);
        if (gw_result.has_value()) ++allowed;
    }

    EXPECT_EQ(allowed, 10);

    server.stop();
}

TEST(ServerStartup, destructor_stops_cleanly) {
    auto config = make_test_config();
    config.grpc_port = 50304;
    config.websocket_port = 9304;

    {
        eyetrack::EyetrackServer server(config);
        server.set_dispatcher(make_test_dispatcher());
        auto result = server.start();
        ASSERT_TRUE(result.has_value()) << result.error().message;
        EXPECT_TRUE(server.is_running());
        // Destructor should call stop()
    }
    // If we get here without hanging, the test passes
    SUCCEED();
}
