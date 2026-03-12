#include <eyetrack/comm/session_manager.hpp>

namespace eyetrack {

Result<void> SessionManager::add(const std::string& client_id, Protocol protocol) {
    std::lock_guard<std::mutex> lock(mu_);
    if (sessions_.contains(client_id)) {
        return make_error(ErrorCode::InvalidInput,
                          "Client already exists: " + client_id);
    }
    sessions_[client_id] = ClientSession{client_id, protocol,
                                          CalibrationState::NotCalibrated};
    return {};
}

void SessionManager::remove(const std::string& client_id) {
    std::lock_guard<std::mutex> lock(mu_);
    sessions_.erase(client_id);
}

std::optional<ClientSession> SessionManager::get(
    const std::string& client_id) const {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = sessions_.find(client_id);
    if (it == sessions_.end()) return std::nullopt;
    return it->second;
}

Result<void> SessionManager::set_calibration(const std::string& client_id,
                                              CalibrationState state) {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = sessions_.find(client_id);
    if (it == sessions_.end()) {
        return make_error(ErrorCode::InvalidInput,
                          "Unknown client: " + client_id);
    }
    it->second.calibration = state;
    return {};
}

uint32_t SessionManager::count() const {
    std::lock_guard<std::mutex> lock(mu_);
    return static_cast<uint32_t>(sessions_.size());
}

bool SessionManager::has(const std::string& client_id) const {
    std::lock_guard<std::mutex> lock(mu_);
    return sessions_.contains(client_id);
}

}  // namespace eyetrack
