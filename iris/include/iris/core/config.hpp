#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <iris/core/error.hpp>

namespace iris {

/// Description of a single pipeline node parsed from YAML config.
struct NodeConfig {
    std::string name;
    std::string class_name;
    std::unordered_map<std::string, std::string> params;

    struct Input {
        std::string name;
        std::string source_node;
        int index = -1;  // -1 means no index
    };
    std::vector<Input> inputs;

    struct Callback {
        std::string class_name;
        std::unordered_map<std::string, std::string> params;
    };
    std::vector<Callback> callbacks;
};

/// Top-level pipeline configuration parsed from YAML.
struct PipelineConfig {
    std::string pipeline_name;
    std::string iris_version;
    std::vector<NodeConfig> nodes;
};

/// Parse a pipeline YAML configuration file.
Result<PipelineConfig> load_pipeline_config(const std::filesystem::path& path);

/// Parse pipeline config from a YAML string.
Result<PipelineConfig> parse_pipeline_config(const std::string& yaml_content);

}  // namespace iris
