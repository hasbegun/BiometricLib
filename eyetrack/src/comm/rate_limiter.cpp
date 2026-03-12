#include <eyetrack/comm/rate_limiter.hpp>

#include <spdlog/spdlog.h>

namespace eyetrack {

RateLimiter::RateLimiter(uint32_t messages_per_second)
    : limit_(messages_per_second) {}

void RateLimiter::refill(Bucket& bucket) const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double>(now - bucket.last_refill).count();
    bucket.tokens = std::min(
        static_cast<double>(limit_),
        bucket.tokens + elapsed * static_cast<double>(limit_));
    bucket.last_refill = now;
}

bool RateLimiter::allow(const std::string& client_id) {
    std::lock_guard<std::mutex> lock(mu_);

    auto it = buckets_.find(client_id);
    if (it == buckets_.end()) {
        auto [inserted, _] = buckets_.emplace(
            client_id,
            Bucket{static_cast<double>(limit_) - 1.0,
                   std::chrono::steady_clock::now()});
        (void)_;
        return true;
    }

    refill(it->second);

    if (it->second.tokens >= 1.0) {
        it->second.tokens -= 1.0;
        return true;
    }

    spdlog::warn("Rate limit exceeded for client '{}'", client_id);
    return false;
}

}  // namespace eyetrack
