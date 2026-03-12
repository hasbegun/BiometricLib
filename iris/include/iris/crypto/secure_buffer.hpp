#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace iris {

/// RAII buffer that securely zeroes memory on destruction.
/// Used for sensitive data like secret keys.
///
/// Features:
/// - Volatile zero on destruction (prevents compiler optimization)
/// - Best-effort mlock() to prevent swapping (warns on failure)
/// - Move-only (no copies)
class SecureBuffer {
  public:
    /// Construct a buffer of `size` bytes, initialized to zero.
    explicit SecureBuffer(size_t size = 0);

    /// Destructor: zeroes memory, then frees.
    ~SecureBuffer();

    /// Move constructor.
    SecureBuffer(SecureBuffer&& other) noexcept;

    /// Move assignment.
    SecureBuffer& operator=(SecureBuffer&& other) noexcept;

    /// No copies.
    SecureBuffer(const SecureBuffer&) = delete;
    SecureBuffer& operator=(const SecureBuffer&) = delete;

    /// Access the buffer contents.
    [[nodiscard]] std::span<uint8_t> data() noexcept;
    [[nodiscard]] std::span<const uint8_t> data() const noexcept;

    /// Buffer size in bytes.
    [[nodiscard]] size_t size() const noexcept { return size_; }

    /// Whether the buffer is empty (size == 0).
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

  private:
    /// Securely zero the buffer contents.
    void secure_zero() noexcept;

    /// Lock/unlock memory pages.
    void lock_memory() noexcept;
    void unlock_memory() noexcept;

    uint8_t* ptr_ = nullptr;
    size_t size_ = 0;
};

}  // namespace iris
