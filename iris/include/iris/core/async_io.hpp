#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <future>
#include <string>
#include <vector>

#include <iris/core/error.hpp>

namespace iris {

/// Policy for retrying model downloads with exponential backoff.
struct DownloadPolicy {
    uint32_t max_retries = 3;
    std::chrono::milliseconds initial_backoff{1000};
    double backoff_multiplier = 4.0;
    std::chrono::milliseconds max_backoff{60000};
    bool verify_checksum = true;
    std::string expected_sha256;  // empty = skip verification
};

/// Async file and network I/O.
///
/// All operations run on a small dedicated I/O thread pool (not the compute
/// pool) so they never block compute work.
class AsyncIO {
public:
    /// Asynchronously read an entire file into memory.
    static std::future<Result<std::vector<uint8_t>>> read_file(
        const std::filesystem::path& path);

    /// Asynchronously write data to a file.
    static std::future<Result<void>> write_file(
        const std::filesystem::path& path,
        std::vector<uint8_t> data);

    /// Download a model from `url`, caching in `cache_dir`.
    /// Returns the local file path. Retries according to `policy`.
    static std::future<Result<std::filesystem::path>> download_model(
        const std::string& url,
        const std::filesystem::path& cache_dir,
        const DownloadPolicy& policy = {});

    /// Asynchronously load and parse a YAML file.
    /// Returns the raw file bytes (caller parses with yaml-cpp).
    static std::future<Result<std::string>> load_yaml(
        const std::filesystem::path& path);
};

}  // namespace iris
