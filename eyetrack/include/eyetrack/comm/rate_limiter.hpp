#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace eyetrack {

/// Token-bucket rate limiter with per-client isolation.
class RateLimiter {
public:
    /// Create a rate limiter with the given limit (messages per second).
    explicit RateLimiter(uint32_t messages_per_second = 60);

    /// Check if a client is allowed to send a message.
    /// Returns true if allowed, false if rate-limited.
    [[nodiscard]] bool allow(const std::string& client_id);

    /// Get the configured limit.
    [[nodiscard]] uint32_t limit() const noexcept { return limit_; }

private:
    struct Bucket {
        double tokens;
        std::chrono::steady_clock::time_point last_refill;
    };

    uint32_t limit_;
    mutable std::mutex mu_;
    std::unordered_map<std::string, Bucket> buckets_;

    void refill(Bucket& bucket) const;
};

}  // namespace eyetrack
