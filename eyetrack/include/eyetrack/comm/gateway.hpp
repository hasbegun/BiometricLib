#pragma once

#include <functional>
#include <memory>
#include <string>

#include <eyetrack/comm/auth_middleware.hpp>
#include <eyetrack/comm/input_validation.hpp>
#include <eyetrack/comm/rate_limiter.hpp>
#include <eyetrack/comm/session_manager.hpp>
#include <eyetrack/core/error.hpp>
#include <eyetrack/core/types.hpp>

namespace eyetrack {

/// Incoming message from any protocol.
struct IncomingMessage {
    std::string client_id;
    std::string auth_token;
    Protocol protocol = Protocol::WebSocket;
    FrameData frame;
};

/// Outgoing result routed back to the originating protocol.
struct OutgoingResult {
    std::string client_id;
    Protocol protocol = Protocol::WebSocket;
    GazeResult gaze;
};

/// Pipeline dispatch function type.
/// Accepts a FrameData, returns a GazeResult.
using PipelineDispatcher =
    std::function<Result<GazeResult>(const FrameData& frame)>;

/// Gateway: unified entry point for all protocol handlers.
///
/// Validates input, authenticates, rate-limits, dispatches to pipeline,
/// and routes results back to the originating protocol.
class Gateway {
public:
    Gateway();

    /// Set the pipeline dispatch function.
    void set_dispatcher(PipelineDispatcher dispatcher);

    /// Set a custom auth middleware (default: NoOpAuthMiddleware).
    void set_auth(std::unique_ptr<AuthMiddleware> auth);

    /// Set rate limiter limit (messages per second per client).
    void set_rate_limit(uint32_t messages_per_second);

    /// Process an incoming message through the full middleware chain.
    /// Returns the result to route back, or an error.
    [[nodiscard]] Result<OutgoingResult> process(const IncomingMessage& msg);

    /// Access the session manager.
    [[nodiscard]] SessionManager& sessions() noexcept { return sessions_; }
    [[nodiscard]] const SessionManager& sessions() const noexcept {
        return sessions_;
    }

private:
    SessionManager sessions_;
    std::unique_ptr<AuthMiddleware> auth_;
    std::unique_ptr<RateLimiter> rate_limiter_;
    PipelineDispatcher dispatcher_;
};

}  // namespace eyetrack
