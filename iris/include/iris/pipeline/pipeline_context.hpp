#pragma once

#include <any>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <iris/core/error.hpp>

namespace iris {

/// Type-erased context map for passing data between pipeline nodes.
///
/// Each pipeline node writes its output under its own name. Downstream
/// nodes read inputs by source-node name with typed accessors.
/// Thread-safe: concurrent reads/writes are protected by a mutex.
class PipelineContext {
  public:
    PipelineContext() : mutex_(std::make_unique<std::mutex>()) {}
    ~PipelineContext() = default;
    PipelineContext(const PipelineContext&) = delete;
    PipelineContext& operator=(const PipelineContext&) = delete;
    PipelineContext(PipelineContext&&) = default;
    PipelineContext& operator=(PipelineContext&&) = default;

    /// Store a value under the given key.
    template <typename T>
    void set(const std::string& key, T value) {
        std::lock_guard lock(*mutex_);
        store_[key] = std::move(value);
    }

    /// Retrieve a const pointer to a value by key with type checking.
    /// Returns nullptr-like error on missing key or type mismatch.
    template <typename T>
    Result<const T*> get(const std::string& key) const {
        std::lock_guard lock(*mutex_);
        auto it = store_.find(key);
        if (it == store_.end()) {
            return make_error(
                ErrorCode::ConfigInvalid,
                "PipelineContext: key not found: " + key);
        }
        const T* ptr = std::any_cast<T>(&it->second);
        if (!ptr) {
            return make_error(
                ErrorCode::ConfigInvalid,
                "PipelineContext: type mismatch for key: " + key,
                std::string("stored type: ") + it->second.type().name());
        }
        return ptr;
    }

    /// Retrieve an element from a pair stored under the given key.
    /// Index is a compile-time constant: 0 -> first, 1 -> second.
    template <size_t Index, typename PairT>
    Result<const std::tuple_element_t<Index, PairT>*>
    get_indexed(const std::string& key) const {
        auto pair_result = get<PairT>(key);
        if (!pair_result) {
            return std::unexpected(pair_result.error());
        }
        return &std::get<Index>(**pair_result);
    }

    /// Check whether a key exists.
    [[nodiscard]] bool has(const std::string& key) const noexcept {
        std::lock_guard lock(*mutex_);
        return store_.contains(key);
    }

    /// Get the raw std::any for a key (for dispatch functions).
    [[nodiscard]] const std::any* get_raw(const std::string& key) const noexcept {
        std::lock_guard lock(*mutex_);
        auto it = store_.find(key);
        return (it != store_.end()) ? &it->second : nullptr;
    }

    /// Return all stored keys.
    [[nodiscard]] std::vector<std::string> keys() const {
        std::lock_guard lock(*mutex_);
        std::vector<std::string> result;
        result.reserve(store_.size());
        for (const auto& [k, _] : store_) {
            result.push_back(k);
        }
        return result;
    }

    /// Number of stored entries.
    [[nodiscard]] size_t size() const noexcept {
        std::lock_guard lock(*mutex_);
        return store_.size();
    }

    /// Remove all entries.
    void clear() noexcept {
        std::lock_guard lock(*mutex_);
        store_.clear();
    }

  private:
    std::unique_ptr<std::mutex> mutex_;
    std::unordered_map<std::string, std::any> store_;
};

}  // namespace iris
