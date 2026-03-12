#pragma once

#include <atomic>

namespace eyetrack {

/// Cooperative cancellation token for pipeline abort.
///
/// Thread-safe. Any thread may call cancel(); any thread may poll is_cancelled().
class CancellationToken {
public:
    CancellationToken() = default;

    CancellationToken(const CancellationToken&) = delete;
    CancellationToken& operator=(const CancellationToken&) = delete;

    /// Signal cancellation. Safe to call from any thread, multiple times.
    void cancel() noexcept { cancelled_.store(true, std::memory_order_release); }

    /// Check if cancellation has been requested.
    [[nodiscard]] bool is_cancelled() const noexcept {
        return cancelled_.load(std::memory_order_acquire);
    }

private:
    std::atomic<bool> cancelled_{false};
};

}  // namespace eyetrack
