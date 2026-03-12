#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include <eyetrack/core/config.hpp>

namespace {

using namespace eyetrack;

const std::filesystem::path kProjectRoot{EYETRACK_PROJECT_ROOT};

// ---- PipelineConfig ----

TEST(PipelineConfig, load_from_yaml) {
    auto result = load_pipeline_config(kProjectRoot / "config" / "pipeline.yaml");
    ASSERT_TRUE(result.has_value()) << result.error().message;

    const auto& cfg = result.value();
    EXPECT_EQ(cfg.pipeline_name, "eyetrack_default");
    EXPECT_EQ(cfg.target_fps, 30U);
    EXPECT_EQ(cfg.frame_width, 640U);
    EXPECT_EQ(cfg.frame_height, 480U);
    EXPECT_FALSE(cfg.nodes.empty());
    EXPECT_EQ(cfg.nodes.size(), 5U);
    EXPECT_EQ(cfg.nodes[0], "face_detection");
    EXPECT_EQ(cfg.nodes[4], "gaze_estimation");
}

TEST(PipelineConfig, default_values) {
    // Parse empty YAML — all defaults should apply
    auto result = parse_pipeline_config("{}");
    ASSERT_TRUE(result.has_value()) << result.error().message;

    const auto& cfg = result.value();
    EXPECT_EQ(cfg.pipeline_name, "eyetrack_default");
    EXPECT_EQ(cfg.target_fps, 30U);
    EXPECT_EQ(cfg.frame_width, 640U);
    EXPECT_EQ(cfg.frame_height, 480U);
    EXPECT_TRUE(cfg.enable_blink_detection);
    EXPECT_FLOAT_EQ(cfg.confidence_threshold, 0.5F);
    EXPECT_TRUE(cfg.nodes.empty());
}

TEST(PipelineConfig, invalid_yaml_returns_error) {
    auto result = parse_pipeline_config("{{{{invalid yaml!!!!!");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ConfigInvalid);
    EXPECT_FALSE(result.error().detail.empty());
}

// ---- ServerConfig ----

TEST(ServerConfig, load_from_yaml) {
    auto result = load_server_config(kProjectRoot / "config" / "server.yaml");
    ASSERT_TRUE(result.has_value()) << result.error().message;

    const auto& cfg = result.value();
    EXPECT_EQ(cfg.host, "0.0.0.0");
    EXPECT_EQ(cfg.grpc_port, 50051);
    EXPECT_EQ(cfg.websocket_port, 8080);
    EXPECT_EQ(cfg.mqtt_port, 1883);
    EXPECT_EQ(cfg.protocol, "websocket");
    EXPECT_EQ(cfg.max_clients, 10U);
}

TEST(ServerConfig, tls_disabled_by_default) {
    auto result = load_server_config(kProjectRoot / "config" / "server.yaml");
    ASSERT_TRUE(result.has_value()) << result.error().message;

    EXPECT_FALSE(result.value().tls.enabled);
}

TEST(TlsConfig, cert_paths_stored) {
    std::string yaml = R"(
host: localhost
tls:
  enabled: true
  cert_path: /etc/ssl/cert.pem
  key_path: /etc/ssl/key.pem
  ca_path: /etc/ssl/ca.pem
)";
    auto result = parse_server_config(yaml);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    const auto& tls = result.value().tls;
    EXPECT_TRUE(tls.enabled);
    EXPECT_EQ(tls.cert_path, "/etc/ssl/cert.pem");
    EXPECT_EQ(tls.key_path, "/etc/ssl/key.pem");
    EXPECT_EQ(tls.ca_path, "/etc/ssl/ca.pem");
}

// ---- EdgeConfig ----

TEST(EdgeConfig, load_from_yaml) {
    auto result = load_edge_config(kProjectRoot / "config" / "edge.yaml");
    ASSERT_TRUE(result.has_value()) << result.error().message;

    const auto& cfg = result.value();
    EXPECT_EQ(cfg.camera_index, 0U);
    EXPECT_EQ(cfg.frame_width, 640U);
    EXPECT_EQ(cfg.frame_height, 480U);
    EXPECT_EQ(cfg.target_fps, 30U);
    EXPECT_EQ(cfg.mqtt_broker, "localhost");
    EXPECT_EQ(cfg.mqtt_port, 1883);
    EXPECT_EQ(cfg.mqtt_topic, "eyetrack/gaze");
    EXPECT_FALSE(cfg.headless);
}

// ---- Round-trip tests ----

TEST(Config, round_trip_pipeline) {
    PipelineConfig original;
    original.pipeline_name = "test_pipeline";
    original.target_fps = 60;
    original.frame_width = 1280;
    original.frame_height = 720;
    original.face_model_path = "models/test_face.onnx";
    original.pupil_model_path = "models/test_pupil.onnx";
    original.enable_blink_detection = false;
    original.confidence_threshold = 0.8F;
    original.nodes = {"node_a", "node_b", "node_c"};

    std::string yaml = serialize_pipeline_config(original);
    auto result = parse_pipeline_config(yaml);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    const auto& reloaded = result.value();
    EXPECT_EQ(reloaded.pipeline_name, original.pipeline_name);
    EXPECT_EQ(reloaded.target_fps, original.target_fps);
    EXPECT_EQ(reloaded.frame_width, original.frame_width);
    EXPECT_EQ(reloaded.frame_height, original.frame_height);
    EXPECT_EQ(reloaded.face_model_path, original.face_model_path);
    EXPECT_EQ(reloaded.pupil_model_path, original.pupil_model_path);
    EXPECT_EQ(reloaded.enable_blink_detection, original.enable_blink_detection);
    EXPECT_FLOAT_EQ(reloaded.confidence_threshold, original.confidence_threshold);
    EXPECT_EQ(reloaded.nodes, original.nodes);
}

TEST(Config, round_trip_server) {
    ServerConfig original;
    original.host = "192.168.1.100";
    original.grpc_port = 9090;
    original.websocket_port = 9091;
    original.mqtt_port = 9092;
    original.protocol = "grpc";
    original.max_clients = 50;
    original.tls.enabled = true;
    original.tls.cert_path = "/tmp/cert.pem";
    original.tls.key_path = "/tmp/key.pem";
    original.tls.ca_path = "/tmp/ca.pem";

    std::string yaml = serialize_server_config(original);
    auto result = parse_server_config(yaml);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    const auto& reloaded = result.value();
    EXPECT_EQ(reloaded.host, original.host);
    EXPECT_EQ(reloaded.grpc_port, original.grpc_port);
    EXPECT_EQ(reloaded.websocket_port, original.websocket_port);
    EXPECT_EQ(reloaded.mqtt_port, original.mqtt_port);
    EXPECT_EQ(reloaded.protocol, original.protocol);
    EXPECT_EQ(reloaded.max_clients, original.max_clients);
    EXPECT_EQ(reloaded.tls.enabled, original.tls.enabled);
    EXPECT_EQ(reloaded.tls.cert_path, original.tls.cert_path);
    EXPECT_EQ(reloaded.tls.key_path, original.tls.key_path);
    EXPECT_EQ(reloaded.tls.ca_path, original.tls.ca_path);
}

// ---- Error cases ----

TEST(Config, missing_file_returns_error) {
    auto result = load_pipeline_config("/nonexistent/path/config.yaml");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, ErrorCode::ConfigInvalid);
}

TEST(Config, empty_file_returns_defaults) {
    // Empty YAML string parses to null node, all defaults apply
    auto result = parse_pipeline_config("");
    ASSERT_TRUE(result.has_value()) << result.error().message;

    const auto& cfg = result.value();
    EXPECT_EQ(cfg.pipeline_name, "eyetrack_default");
    EXPECT_EQ(cfg.target_fps, 30U);
}

}  // namespace
