#pragma once

#include <memory>
#include <string>

#include <eyetrack/core/error.hpp>

namespace eyetrack {

/// Identity returned by auth middleware.
struct ClientIdentity {
    std::string client_id;
    std::string role = "user";
};

/// Abstract auth middleware interface.
class AuthMiddleware {
public:
    virtual ~AuthMiddleware() = default;

    /// Authenticate a request. Returns client identity on success.
    [[nodiscard]] virtual Result<ClientIdentity> authenticate(
        const std::string& token) = 0;
};

/// No-op auth middleware that accepts all requests (for development).
class NoOpAuthMiddleware : public AuthMiddleware {
public:
    [[nodiscard]] Result<ClientIdentity> authenticate(
        const std::string& token) override;
};

/// Factory: create default auth middleware.
[[nodiscard]] std::unique_ptr<AuthMiddleware> make_default_auth();

}  // namespace eyetrack
