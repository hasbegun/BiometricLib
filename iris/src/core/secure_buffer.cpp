#include <iris/crypto/secure_buffer.hpp>

#include <cstdlib>
#include <cstring>
#include <new>
#include <utility>

#ifdef __has_include
#if __has_include(<sys/mman.h>)
#include <sys/mman.h>
#define IRIS_HAS_MLOCK 1
#endif
#endif

namespace iris {

namespace {

/// Securely zero memory — prevent compiler from optimizing away the memset.
/// Uses explicit_bzero where available, otherwise volatile memset.
void volatile_zero(void* ptr, size_t size) noexcept {
    if (!ptr || size == 0) return;
#if defined(__GLIBC__) && defined(__GLIBC_MINOR__) && \
    (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 25))
    explicit_bzero(ptr, size);
#else
    // Volatile pointer prevents the compiler from optimizing away the write
    volatile unsigned char* vp = static_cast<volatile unsigned char*>(ptr);
    for (size_t i = 0; i < size; ++i) {
        vp[i] = 0;
    }
#endif
}

}  // namespace

SecureBuffer::SecureBuffer(size_t size) : size_(size) {
    if (size_ == 0) return;

    // Use aligned allocation for potential SIMD use
    ptr_ = static_cast<uint8_t*>(std::aligned_alloc(64, ((size_ + 63) / 64) * 64));
    if (!ptr_) throw std::bad_alloc();

    std::memset(ptr_, 0, size_);
    lock_memory();
}

SecureBuffer::~SecureBuffer() {
    secure_zero();
    unlock_memory();
    std::free(ptr_);
}

SecureBuffer::SecureBuffer(SecureBuffer&& other) noexcept
    : ptr_(std::exchange(other.ptr_, nullptr)),
      size_(std::exchange(other.size_, 0)) {}

SecureBuffer& SecureBuffer::operator=(SecureBuffer&& other) noexcept {
    if (this != &other) {
        secure_zero();
        unlock_memory();
        std::free(ptr_);
        ptr_ = std::exchange(other.ptr_, nullptr);
        size_ = std::exchange(other.size_, 0);
    }
    return *this;
}

std::span<uint8_t> SecureBuffer::data() noexcept {
    return {ptr_, size_};
}

std::span<const uint8_t> SecureBuffer::data() const noexcept {
    return {ptr_, size_};
}

void SecureBuffer::secure_zero() noexcept {
    volatile_zero(ptr_, size_);
}

void SecureBuffer::lock_memory() noexcept {
#ifdef IRIS_HAS_MLOCK
    if (ptr_ && size_ > 0) {
        // Best-effort: don't fail if mlock is not available or limit exceeded
        mlock(ptr_, size_);
    }
#endif
}

void SecureBuffer::unlock_memory() noexcept {
#ifdef IRIS_HAS_MLOCK
    if (ptr_ && size_ > 0) {
        munlock(ptr_, size_);
    }
#endif
}

}  // namespace iris
