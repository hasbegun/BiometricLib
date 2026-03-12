#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <eyetrack/core/error.hpp>

namespace eyetrack {

// ---- TLS configuration ----

struct TlsConfig {
    bool enabled = false;
    std::string cert_path;
    std::string key_path;
    std::string ca_path;
};

// ---- Pipeline configuration ----

struct PipelineConfig {
    std::string pipeline_name = "eyetrack_default";
    uint32_t target_fps = 30;
    uint32_t frame_width = 640;
    uint32_t frame_height = 480;
    std::string face_model_path = "models/face_detection.onnx";
    std::string pupil_model_path = "models/pupil_detection.onnx";
    std::vector<std::string> nodes;  // ordered node names for the pipeline
    bool enable_blink_detection = true;
    float confidence_threshold = 0.5F;
};

// ---- Server configuration ----

struct ServerConfig {
    std::string host = "0.0.0.0";
    uint16_t grpc_port = 50051;
    uint16_t websocket_port = 8080;
    uint16_t mqtt_port = 1883;
    std::string protocol = "websocket";  // "grpc", "websocket", "mqtt"
    uint32_t max_clients = 10;
    TlsConfig tls;
};

// ---- Edge device configuration ----

struct EdgeConfig {
    uint32_t camera_index = 0;
    uint32_t frame_width = 640;
    uint32_t frame_height = 480;
    uint32_t target_fps = 30;
    std::string mqtt_broker = "localhost";
    uint16_t mqtt_port = 1883;
    std::string mqtt_topic = "eyetrack/gaze";
    bool headless = false;
};

// ---- YAML load/save functions ----

Result<PipelineConfig> load_pipeline_config(const std::filesystem::path& path);
Result<PipelineConfig> parse_pipeline_config(const std::string& yaml_content);

Result<ServerConfig> load_server_config(const std::filesystem::path& path);
Result<ServerConfig> parse_server_config(const std::string& yaml_content);

Result<EdgeConfig> load_edge_config(const std::filesystem::path& path);
Result<EdgeConfig> parse_edge_config(const std::string& yaml_content);

std::string serialize_pipeline_config(const PipelineConfig& config);
std::string serialize_server_config(const ServerConfig& config);

}  // namespace eyetrack
