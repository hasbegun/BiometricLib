#include <eyetrack/core/config.hpp>

#include <fstream>
#include <sstream>

#include <yaml-cpp/yaml.h>

namespace eyetrack {

// ---- Pipeline config ----

Result<PipelineConfig> parse_pipeline_config(const std::string& yaml_content) {
    try {
        YAML::Node root = YAML::Load(yaml_content);
        PipelineConfig cfg;

        if (root["pipeline_name"]) {
            cfg.pipeline_name = root["pipeline_name"].as<std::string>();
        }
        if (root["target_fps"]) {
            cfg.target_fps = root["target_fps"].as<uint32_t>();
        }
        if (root["frame_width"]) {
            cfg.frame_width = root["frame_width"].as<uint32_t>();
        }
        if (root["frame_height"]) {
            cfg.frame_height = root["frame_height"].as<uint32_t>();
        }
        if (root["face_model_path"]) {
            cfg.face_model_path = root["face_model_path"].as<std::string>();
        }
        if (root["pupil_model_path"]) {
            cfg.pupil_model_path = root["pupil_model_path"].as<std::string>();
        }
        if (root["enable_blink_detection"]) {
            cfg.enable_blink_detection = root["enable_blink_detection"].as<bool>();
        }
        if (root["confidence_threshold"]) {
            cfg.confidence_threshold = root["confidence_threshold"].as<float>();
        }
        if (root["nodes"] && root["nodes"].IsSequence()) {
            for (const auto& node : root["nodes"]) {
                cfg.nodes.push_back(node.as<std::string>());
            }
        }

        return cfg;
    } catch (const YAML::Exception& e) {
        return make_error(ErrorCode::ConfigInvalid, "YAML parse error", e.what());
    }
}

Result<PipelineConfig> load_pipeline_config(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        return make_error(ErrorCode::ConfigInvalid, "Failed to open config file", path.string());
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return parse_pipeline_config(ss.str());
}

// ---- Server config ----

namespace {

TlsConfig parse_tls(const YAML::Node& node) {
    TlsConfig tls;
    if (!node || !node.IsMap()) return tls;

    if (node["enabled"]) tls.enabled = node["enabled"].as<bool>();
    if (node["cert_path"]) tls.cert_path = node["cert_path"].as<std::string>();
    if (node["key_path"]) tls.key_path = node["key_path"].as<std::string>();
    if (node["ca_path"]) tls.ca_path = node["ca_path"].as<std::string>();

    return tls;
}

}  // namespace

Result<ServerConfig> parse_server_config(const std::string& yaml_content) {
    try {
        YAML::Node root = YAML::Load(yaml_content);
        ServerConfig cfg;

        if (root["host"]) cfg.host = root["host"].as<std::string>();
        if (root["grpc_port"]) cfg.grpc_port = root["grpc_port"].as<uint16_t>();
        if (root["websocket_port"]) cfg.websocket_port = root["websocket_port"].as<uint16_t>();
        if (root["mqtt_port"]) cfg.mqtt_port = root["mqtt_port"].as<uint16_t>();
        if (root["protocol"]) cfg.protocol = root["protocol"].as<std::string>();
        if (root["max_clients"]) cfg.max_clients = root["max_clients"].as<uint32_t>();
        if (root["tls"]) cfg.tls = parse_tls(root["tls"]);

        return cfg;
    } catch (const YAML::Exception& e) {
        return make_error(ErrorCode::ConfigInvalid, "YAML parse error", e.what());
    }
}

Result<ServerConfig> load_server_config(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        return make_error(ErrorCode::ConfigInvalid, "Failed to open config file", path.string());
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return parse_server_config(ss.str());
}

// ---- Edge config ----

Result<EdgeConfig> parse_edge_config(const std::string& yaml_content) {
    try {
        YAML::Node root = YAML::Load(yaml_content);
        EdgeConfig cfg;

        if (root["camera_index"]) cfg.camera_index = root["camera_index"].as<uint32_t>();
        if (root["frame_width"]) cfg.frame_width = root["frame_width"].as<uint32_t>();
        if (root["frame_height"]) cfg.frame_height = root["frame_height"].as<uint32_t>();
        if (root["target_fps"]) cfg.target_fps = root["target_fps"].as<uint32_t>();
        if (root["mqtt_broker"]) cfg.mqtt_broker = root["mqtt_broker"].as<std::string>();
        if (root["mqtt_port"]) cfg.mqtt_port = root["mqtt_port"].as<uint16_t>();
        if (root["mqtt_topic"]) cfg.mqtt_topic = root["mqtt_topic"].as<std::string>();
        if (root["headless"]) cfg.headless = root["headless"].as<bool>();

        return cfg;
    } catch (const YAML::Exception& e) {
        return make_error(ErrorCode::ConfigInvalid, "YAML parse error", e.what());
    }
}

Result<EdgeConfig> load_edge_config(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        return make_error(ErrorCode::ConfigInvalid, "Failed to open config file", path.string());
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return parse_edge_config(ss.str());
}

// ---- Serialization ----

std::string serialize_pipeline_config(const PipelineConfig& config) {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "pipeline_name" << YAML::Value << config.pipeline_name;
    out << YAML::Key << "target_fps" << YAML::Value << config.target_fps;
    out << YAML::Key << "frame_width" << YAML::Value << config.frame_width;
    out << YAML::Key << "frame_height" << YAML::Value << config.frame_height;
    out << YAML::Key << "face_model_path" << YAML::Value << config.face_model_path;
    out << YAML::Key << "pupil_model_path" << YAML::Value << config.pupil_model_path;
    out << YAML::Key << "enable_blink_detection" << YAML::Value << config.enable_blink_detection;
    out << YAML::Key << "confidence_threshold" << YAML::Value << config.confidence_threshold;
    out << YAML::Key << "nodes" << YAML::Value << YAML::BeginSeq;
    for (const auto& node : config.nodes) {
        out << node;
    }
    out << YAML::EndSeq;
    out << YAML::EndMap;
    return out.c_str();
}

std::string serialize_server_config(const ServerConfig& config) {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "host" << YAML::Value << config.host;
    out << YAML::Key << "grpc_port" << YAML::Value << config.grpc_port;
    out << YAML::Key << "websocket_port" << YAML::Value << config.websocket_port;
    out << YAML::Key << "mqtt_port" << YAML::Value << config.mqtt_port;
    out << YAML::Key << "protocol" << YAML::Value << config.protocol;
    out << YAML::Key << "max_clients" << YAML::Value << config.max_clients;
    out << YAML::Key << "tls" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "enabled" << YAML::Value << config.tls.enabled;
    out << YAML::Key << "cert_path" << YAML::Value << config.tls.cert_path;
    out << YAML::Key << "key_path" << YAML::Value << config.tls.key_path;
    out << YAML::Key << "ca_path" << YAML::Value << config.tls.ca_path;
    out << YAML::EndMap;
    out << YAML::EndMap;
    return out.c_str();
}

}  // namespace eyetrack
