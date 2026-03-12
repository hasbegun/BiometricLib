#include <gtest/gtest.h>

#ifdef EYETRACK_HAS_WEBSOCKET

#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <eyetrack/comm/websocket_server.hpp>
#include <eyetrack/core/config.hpp>

namespace beast = boost::beast;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace {

/// Helper: connect a plain WS client and return the stream.
auto make_client(net::io_context& ioc, uint16_t port)
    -> beast::websocket::stream<tcp::socket> {
    tcp::resolver resolver(ioc);
    auto results = resolver.resolve("127.0.0.1", std::to_string(port));

    beast::websocket::stream<tcp::socket> ws(ioc);
    net::connect(ws.next_layer(), results);
    ws.handshake("127.0.0.1:" + std::to_string(port), "/");
    ws.binary(true);
    return ws;
}

}  // namespace

TEST(WebSocketStreaming, single_client_echo) {
    eyetrack::ServerConfig config;
    config.websocket_port = 9200;
    config.tls.enabled = false;

    eyetrack::WebSocketServer server(config);
    server.set_message_handler(
        [](const std::string& data,
           const std::string& /*client_id*/) -> std::string {
            return "echo:" + data;
        });

    auto result = server.start();
    ASSERT_TRUE(result.has_value()) << result.error().message;

    // Give server a moment to start accepting
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    net::io_context ioc;
    auto ws = make_client(ioc, 9200);

    // Send a message
    std::string msg = "hello";
    ws.write(net::buffer(msg));

    // Read response
    beast::flat_buffer buf;
    ws.read(buf);
    auto response = beast::buffers_to_string(buf.data());
    EXPECT_EQ(response, "echo:hello");

    ws.close(beast::websocket::close_code::normal);
    server.stop();
}

TEST(WebSocketStreaming, concurrent_10_clients) {
    eyetrack::ServerConfig config;
    config.websocket_port = 9201;
    config.tls.enabled = false;

    eyetrack::WebSocketServer server(config);
    server.set_message_handler(
        [](const std::string& data,
           const std::string& /*client_id*/) -> std::string {
            return data;
        });

    auto result = server.start();
    ASSERT_TRUE(result.has_value()) << result.error().message;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    constexpr int kNumClients = 10;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < kNumClients; ++i) {
        threads.emplace_back([i, &success_count] {
            try {
                net::io_context ioc;
                auto ws = make_client(ioc, 9201);

                std::string msg = "client_" + std::to_string(i);
                ws.write(net::buffer(msg));

                beast::flat_buffer buf;
                ws.read(buf);
                auto resp = beast::buffers_to_string(buf.data());
                if (resp == msg) {
                    success_count.fetch_add(1, std::memory_order_relaxed);
                }

                ws.close(beast::websocket::close_code::normal);
            } catch (...) {
                // Connection failure counts as not-success
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count.load(), kNumClients);
    server.stop();
}

TEST(WebSocketStreaming, binary_protobuf_payload) {
    eyetrack::ServerConfig config;
    config.websocket_port = 9202;
    config.tls.enabled = false;

    eyetrack::WebSocketServer server(config);
    server.set_message_handler(
        [](const std::string& data,
           const std::string& /*client_id*/) -> std::string {
            // Echo binary data as-is
            return data;
        });

    auto result = server.start();
    ASSERT_TRUE(result.has_value()) << result.error().message;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    net::io_context ioc;
    auto ws = make_client(ioc, 9202);

    // Send binary data (simulated protobuf)
    std::string binary_data = {'\x08', '\x01', '\x15', '\x00', '\x00', '\x80', '\x3f'};
    ws.write(net::buffer(binary_data));

    beast::flat_buffer buf;
    ws.read(buf);
    auto response = beast::buffers_to_string(buf.data());
    EXPECT_EQ(response, binary_data);

    ws.close(beast::websocket::close_code::normal);
    server.stop();
}

TEST(WebSocketStreaming, client_disconnect_handled) {
    eyetrack::ServerConfig config;
    config.websocket_port = 9203;
    config.tls.enabled = false;

    eyetrack::WebSocketServer server(config);
    server.set_message_handler(
        [](const std::string& data,
           const std::string& /*client_id*/) -> std::string {
            return data;
        });

    auto result = server.start();
    ASSERT_TRUE(result.has_value()) << result.error().message;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    {
        net::io_context ioc;
        auto ws = make_client(ioc, 9203);
        // Let ws go out of scope — abrupt disconnect
    }

    // Server should still be running after client disconnect
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(server.is_running());

    server.stop();
}

TEST(WebSocketStreaming, reconnect_after_disconnect) {
    eyetrack::ServerConfig config;
    config.websocket_port = 9204;
    config.tls.enabled = false;

    eyetrack::WebSocketServer server(config);
    server.set_message_handler(
        [](const std::string& data,
           const std::string& /*client_id*/) -> std::string {
            return "ok";
        });

    auto result = server.start();
    ASSERT_TRUE(result.has_value()) << result.error().message;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // First connection
    {
        net::io_context ioc;
        auto ws = make_client(ioc, 9204);
        ws.write(net::buffer(std::string("first")));
        beast::flat_buffer buf;
        ws.read(buf);
        EXPECT_EQ(beast::buffers_to_string(buf.data()), "ok");
        ws.close(beast::websocket::close_code::normal);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Reconnect
    {
        net::io_context ioc;
        auto ws = make_client(ioc, 9204);
        ws.write(net::buffer(std::string("second")));
        beast::flat_buffer buf;
        ws.read(buf);
        EXPECT_EQ(beast::buffers_to_string(buf.data()), "ok");
        ws.close(beast::websocket::close_code::normal);
    }

    server.stop();
}

TEST(WebSocketStreaming, streaming_30fps_throughput) {
    eyetrack::ServerConfig config;
    config.websocket_port = 9205;
    config.tls.enabled = false;

    eyetrack::WebSocketServer server(config);
    server.set_message_handler(
        [](const std::string& data,
           const std::string& /*client_id*/) -> std::string {
            return data;
        });

    auto result = server.start();
    ASSERT_TRUE(result.has_value()) << result.error().message;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    net::io_context ioc;
    auto ws = make_client(ioc, 9205);

    // Simulate 30fps for 1 second (30 frames)
    constexpr int kFrames = 30;
    std::string frame_data(256, 'X');  // ~256 byte payload per frame

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < kFrames; ++i) {
        ws.write(net::buffer(frame_data));
        beast::flat_buffer buf;
        ws.read(buf);
    }
    auto t1 = std::chrono::steady_clock::now();

    auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    // 30 round-trips should complete well under 2 seconds on localhost
    EXPECT_LT(elapsed_ms, 2000);

    ws.close(beast::websocket::close_code::normal);
    server.stop();
}

TEST(WebSocketStreaming, message_handler_receives_client_id) {
    eyetrack::ServerConfig config;
    config.websocket_port = 9206;
    config.tls.enabled = false;

    std::string captured_client_id;
    eyetrack::WebSocketServer server(config);
    server.set_message_handler(
        [&captured_client_id](const std::string& /*data*/,
                              const std::string& client_id) -> std::string {
            captured_client_id = client_id;
            return "ack";
        });

    auto result = server.start();
    ASSERT_TRUE(result.has_value()) << result.error().message;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    net::io_context ioc;
    auto ws = make_client(ioc, 9206);
    ws.write(net::buffer(std::string("ping")));

    beast::flat_buffer buf;
    ws.read(buf);

    // Client ID should be non-empty (ip:port format)
    EXPECT_FALSE(captured_client_id.empty());
    EXPECT_NE(captured_client_id, "unknown");

    ws.close(beast::websocket::close_code::normal);
    server.stop();
}

#else

TEST(WebSocketStreaming, skipped_without_websocket) {
    GTEST_SKIP() << "WebSocket support not enabled";
}

#endif  // EYETRACK_HAS_WEBSOCKET
