#include <eyetrack/comm/gateway.hpp>

#include <utility>

namespace eyetrack {

Gateway::Gateway()
    : auth_(make_default_auth()),
      rate_limiter_(std::make_unique<RateLimiter>(60)) {}

void Gateway::set_dispatcher(PipelineDispatcher dispatcher) {
    dispatcher_ = std::move(dispatcher);
}

void Gateway::set_auth(std::unique_ptr<AuthMiddleware> auth) {
    auth_ = std::move(auth);
}

void Gateway::set_rate_limit(uint32_t messages_per_second) {
    rate_limiter_ = std::make_unique<RateLimiter>(messages_per_second);
}

Result<OutgoingResult> Gateway::process(const IncomingMessage& msg) {
    // 1. Authenticate
    auto auth_result = auth_->authenticate(msg.auth_token);
    if (!auth_result.has_value()) {
        return std::unexpected(auth_result.error());
    }

    // 2. Validate client ID
    auto id_result = InputValidator::validate_client_id(msg.client_id);
    if (!id_result.has_value()) {
        return std::unexpected(id_result.error());
    }

    // 3. Validate frame
    auto dim_result = InputValidator::validate_frame_dimensions(
        static_cast<uint32_t>(msg.frame.frame.cols),
        static_cast<uint32_t>(msg.frame.frame.rows));
    if (!dim_result.has_value()) {
        return std::unexpected(dim_result.error());
    }

    auto size_result = InputValidator::validate_frame_size(msg.frame.frame.total() *
                                                            msg.frame.frame.elemSize());
    if (!size_result.has_value()) {
        return std::unexpected(size_result.error());
    }

    // 4. Rate limit
    if (!rate_limiter_->allow(msg.client_id)) {
        return make_error(ErrorCode::RateLimited,
                          "Rate limit exceeded for client: " + msg.client_id);
    }

    // 5. Ensure session exists
    if (!sessions_.has(msg.client_id)) {
        auto add_result = sessions_.add(msg.client_id, msg.protocol);
        if (!add_result.has_value()) {
            return std::unexpected(add_result.error());
        }
    }

    // 6. Dispatch to pipeline
    if (!dispatcher_) {
        return make_error(ErrorCode::ConfigInvalid, "No pipeline dispatcher set");
    }

    auto gaze_result = dispatcher_(msg.frame);
    if (!gaze_result.has_value()) {
        return std::unexpected(gaze_result.error());
    }

    // 7. Route result back
    OutgoingResult result;
    result.client_id = msg.client_id;
    result.protocol = msg.protocol;
    result.gaze = gaze_result.value();
    return result;
}

}  // namespace eyetrack
