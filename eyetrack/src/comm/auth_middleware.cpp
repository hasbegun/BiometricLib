#include <eyetrack/comm/auth_middleware.hpp>

namespace eyetrack {

Result<ClientIdentity> NoOpAuthMiddleware::authenticate(
    const std::string& token) {
    // Accept all requests — return identity based on token or default
    ClientIdentity identity;
    identity.client_id = token.empty() ? "anonymous" : token;
    identity.role = "user";
    return identity;
}

std::unique_ptr<AuthMiddleware> make_default_auth() {
    return std::make_unique<NoOpAuthMiddleware>();
}

}  // namespace eyetrack
