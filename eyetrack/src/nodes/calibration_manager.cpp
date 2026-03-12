#include <eyetrack/nodes/calibration_manager.hpp>

#include <fstream>
#include <sstream>

#include <yaml-cpp/yaml.h>

#include <eyetrack/core/node_registry.hpp>

namespace eyetrack {

const std::vector<std::string> CalibrationManager::kDependencies = {
    "eyetrack.PupilDetector"};

CalibrationManager::CalibrationManager() = default;

CalibrationManager::CalibrationManager(const NodeParams& /*params*/) {}

void CalibrationManager::start_calibration(const std::string& user_id) {
    profile_ = CalibrationProfile{};
    profile_.user_id = user_id;
    transform_ = CalibrationTransform{};
    calibrated_ = false;
}

void CalibrationManager::add_point(Point2D screen_point, PupilInfo pupil) {
    profile_.screen_points.push_back(screen_point);
    profile_.pupil_observations.push_back(pupil);
}

size_t CalibrationManager::point_count() const noexcept {
    return profile_.screen_points.size();
}

Result<CalibrationTransform> CalibrationManager::finish_calibration() {
    // Extract pupil centers as Point2D
    std::vector<Point2D> pupil_positions;
    pupil_positions.reserve(profile_.pupil_observations.size());
    for (const auto& obs : profile_.pupil_observations) {
        pupil_positions.push_back(obs.center);
    }

    auto result = least_squares_fit(pupil_positions, profile_.screen_points);
    if (!result.has_value()) {
        return std::unexpected(result.error());
    }

    transform_.transform_left = *result;
    transform_.transform_right = *result;  // same mapping for both eyes
    calibrated_ = true;

    // Cache the profile
    profiles_[profile_.user_id] = profile_;
    transforms_[profile_.user_id] = transform_;

    return transform_;
}

// --- YAML serialization helpers ---

namespace {

void emit_matrix(YAML::Emitter& out, const std::string& key,
                 const Eigen::MatrixXf& mat) {
    out << YAML::Key << key;
    out << YAML::BeginMap;
    out << YAML::Key << "rows" << YAML::Value << mat.rows();
    out << YAML::Key << "cols" << YAML::Value << mat.cols();
    out << YAML::Key << "data" << YAML::Value << YAML::Flow << YAML::BeginSeq;
    for (Eigen::Index r = 0; r < mat.rows(); ++r) {
        for (Eigen::Index c = 0; c < mat.cols(); ++c) {
            out << mat(r, c);
        }
    }
    out << YAML::EndSeq;
    out << YAML::EndMap;
}

Eigen::MatrixXf parse_matrix(const YAML::Node& node) {
    auto rows = node["rows"].as<int>();
    auto cols = node["cols"].as<int>();
    Eigen::MatrixXf mat(rows, cols);
    const auto& data = node["data"];
    int idx = 0;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            mat(r, c) = data[static_cast<size_t>(idx++)].as<float>();
        }
    }
    return mat;
}

}  // namespace

Result<void> CalibrationManager::save_profile(const std::string& path) const {
    if (!calibrated_) {
        return make_error(ErrorCode::CalibrationFailed,
                          "No calibration to save");
    }

    YAML::Emitter out;
    out << YAML::BeginMap;

    out << YAML::Key << "user_id" << YAML::Value << profile_.user_id;

    // Screen points
    out << YAML::Key << "screen_points" << YAML::Value << YAML::BeginSeq;
    for (const auto& sp : profile_.screen_points) {
        out << YAML::Flow << YAML::BeginSeq
            << sp.x << sp.y << YAML::EndSeq;
    }
    out << YAML::EndSeq;

    // Pupil observations
    out << YAML::Key << "pupil_observations" << YAML::Value << YAML::BeginSeq;
    for (const auto& po : profile_.pupil_observations) {
        out << YAML::Flow << YAML::BeginMap;
        out << YAML::Key << "cx" << YAML::Value << po.center.x;
        out << YAML::Key << "cy" << YAML::Value << po.center.y;
        out << YAML::Key << "r" << YAML::Value << po.radius;
        out << YAML::Key << "conf" << YAML::Value << po.confidence;
        out << YAML::EndMap;
    }
    out << YAML::EndSeq;

    // Transform matrices
    emit_matrix(out, "transform_left", transform_.transform_left);
    emit_matrix(out, "transform_right", transform_.transform_right);

    out << YAML::EndMap;

    std::ofstream file(path);
    if (!file.is_open()) {
        return make_error(ErrorCode::ConfigInvalid,
                          "Cannot open file for writing: " + path);
    }
    file << out.c_str();
    return {};
}

Result<void> CalibrationManager::load_profile(const std::string& path) {
    try {
        YAML::Node root = YAML::LoadFile(path);

        profile_ = CalibrationProfile{};
        profile_.user_id = root["user_id"].as<std::string>();

        for (const auto& sp : root["screen_points"]) {
            profile_.screen_points.push_back(
                {sp[0].as<float>(), sp[1].as<float>()});
        }

        for (const auto& po : root["pupil_observations"]) {
            PupilInfo pi;
            pi.center = {po["cx"].as<float>(), po["cy"].as<float>()};
            pi.radius = po["r"].as<float>();
            pi.confidence = po["conf"].as<float>();
            profile_.pupil_observations.push_back(pi);
        }

        transform_.transform_left = parse_matrix(root["transform_left"]);
        transform_.transform_right = parse_matrix(root["transform_right"]);
        calibrated_ = true;

        // Cache
        profiles_[profile_.user_id] = profile_;
        transforms_[profile_.user_id] = transform_;

        return {};
    } catch (const YAML::Exception& e) {
        return make_error(ErrorCode::ConfigInvalid,
                          "Failed to load calibration profile", e.what());
    }
}

Result<CalibrationProfile> CalibrationManager::get_profile(
    const std::string& user_id) const {
    auto it = profiles_.find(user_id);
    if (it == profiles_.end()) {
        return make_error(ErrorCode::CalibrationNotLoaded,
                          "No profile for user: " + user_id);
    }
    return it->second;
}

Result<void> CalibrationManager::execute(
    std::unordered_map<std::string, std::any>& context) const {
    // Pipeline execute is a no-op for now; calibration is driven
    // by explicit API calls (start/add_point/finish).
    // If a calibration_transform exists, expose it in the context.
    if (calibrated_) {
        context["calibration_transform"] = transform_;
        context["calibration_profile"] = profile_;
    }
    return {};
}

const std::vector<std::string>& CalibrationManager::dependencies()
    const noexcept {
    return kDependencies;
}

EYETRACK_REGISTER_NODE("eyetrack.CalibrationManager", CalibrationManager);

}  // namespace eyetrack
