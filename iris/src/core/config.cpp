#include <iris/core/config.hpp>

#include <fstream>
#include <sstream>

#include <yaml-cpp/yaml.h>

namespace iris {

namespace {

/// Recursively flatten YAML params into string key-value pairs.
/// Nested maps become dot-separated keys: "circle_extrapolation.class_name".
void flatten_params(
    const YAML::Node& node,
    const std::string& prefix,
    std::unordered_map<std::string, std::string>& out) {
    if (!node.IsDefined() || node.IsNull()) return;

    if (node.IsMap()) {
        for (const auto& kv : node) {
            auto key = prefix.empty() ? kv.first.as<std::string>()
                                      : prefix + "." + kv.first.as<std::string>();
            flatten_params(kv.second, key, out);
        }
    } else if (node.IsSequence()) {
        // Check if all elements are scalars — store as comma-separated string
        bool all_scalars = true;
        for (size_t i = 0; i < node.size(); ++i) {
            if (!node[i].IsScalar()) {
                all_scalars = false;
                break;
            }
        }

        if (all_scalars && node.size() > 0) {
            // Simple scalar list → comma-separated string under the prefix key
            std::string csv;
            for (size_t i = 0; i < node.size(); ++i) {
                if (i > 0) csv += ",";
                csv += node[i].as<std::string>();
            }
            out[prefix] = csv;
        }

        // Always also store indexed entries for complex sub-objects
        for (size_t i = 0; i < node.size(); ++i) {
            if (node[i].IsMap() && node[i]["class_name"]) {
                auto idx_prefix = prefix + "[" + std::to_string(i) + "]";
                flatten_params(node[i], idx_prefix, out);
            } else if (node[i].IsScalar()) {
                auto idx_prefix = prefix + "[" + std::to_string(i) + "]";
                out[idx_prefix] = node[i].as<std::string>();
            } else {
                auto idx_prefix = prefix + "[" + std::to_string(i) + "]";
                flatten_params(node[i], idx_prefix, out);
            }
        }
    } else {
        out[prefix] = node.as<std::string>();
    }
}

NodeConfig parse_node(const YAML::Node& node) {
    NodeConfig cfg;
    cfg.name = node["name"].as<std::string>();

    const auto& algo = node["algorithm"];
    cfg.class_name = algo["class_name"].as<std::string>();

    if (algo["params"] && algo["params"].IsMap()) {
        flatten_params(algo["params"], "", cfg.params);
    }

    if (node["inputs"] && node["inputs"].IsSequence()) {
        for (const auto& input : node["inputs"]) {
            NodeConfig::Input inp;
            inp.name = input["name"].as<std::string>();

            // source_node can be a string or a sequence of {name, index} objects
            const auto& src = input["source_node"];
            if (src.IsScalar()) {
                inp.source_node = src.as<std::string>();
            } else if (src.IsSequence()) {
                // Multi-source input (e.g., noise_masks_aggregation)
                // Store as comma-separated "name:index" pairs
                std::string combined;
                for (const auto& s : src) {
                    if (!combined.empty()) combined += ",";
                    combined += s["name"].as<std::string>();
                    if (s["index"]) {
                        combined += ":" + std::to_string(s["index"].as<int>());
                    }
                }
                inp.source_node = combined;
            }

            if (input["index"]) {
                inp.index = input["index"].as<int>();
            }
            cfg.inputs.push_back(std::move(inp));
        }
    }

    if (node["callbacks"] && node["callbacks"].IsSequence()) {
        for (const auto& cb : node["callbacks"]) {
            NodeConfig::Callback callback;
            callback.class_name = cb["class_name"].as<std::string>();
            if (cb["params"] && cb["params"].IsMap()) {
                flatten_params(cb["params"], "", callback.params);
            }
            cfg.callbacks.push_back(std::move(callback));
        }
    }

    return cfg;
}

}  // namespace

Result<PipelineConfig> parse_pipeline_config(const std::string& yaml_content) {
    try {
        YAML::Node root = YAML::Load(yaml_content);

        PipelineConfig config;

        if (const auto& meta = root["metadata"]) {
            if (meta["pipeline_name"]) {
                config.pipeline_name = meta["pipeline_name"].as<std::string>();
            }
            if (meta["iris_version"]) {
                config.iris_version = meta["iris_version"].as<std::string>();
            }
        } else {
            return make_error(ErrorCode::ConfigInvalid,
                              "Missing 'metadata' key in pipeline config");
        }

        if (!root["pipeline"] || !root["pipeline"].IsSequence()) {
            return make_error(ErrorCode::ConfigInvalid,
                              "Missing or invalid 'pipeline' key in config");
        }

        for (const auto& node : root["pipeline"]) {
            config.nodes.push_back(parse_node(node));
        }

        return config;
    } catch (const YAML::Exception& e) {
        return make_error(ErrorCode::ConfigInvalid, "YAML parse error", e.what());
    }
}

Result<PipelineConfig> load_pipeline_config(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        return make_error(ErrorCode::IoFailed, "Failed to open config file", path.string());
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return parse_pipeline_config(ss.str());
}

}  // namespace iris
