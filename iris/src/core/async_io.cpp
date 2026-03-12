#include <iris/core/async_io.hpp>

#include <condition_variable>
#include <deque>
#include <fstream>
#include <functional>
#include <future>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

namespace iris {

namespace {

/// Small dedicated I/O thread pool (2 threads).
class IoPool {
public:
    static IoPool& instance() {
        static IoPool pool;
        return pool;
    }

    template <typename F>
    auto submit(F&& f) -> std::future<std::invoke_result_t<F>> {
        using R = std::invoke_result_t<F>;
        auto task = std::make_shared<std::packaged_task<R()>>(std::forward<F>(f));
        auto future = task->get_future();
        {
            std::lock_guard lock(mutex_);
            tasks_.push_back([task = std::move(task)]() { (*task)(); });
        }
        cv_.notify_one();
        return future;
    }

    ~IoPool() {
        stop_ = true;
        cv_.notify_all();
    }

private:
    IoPool() {
        for (int i = 0; i < 2; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock lock(mutex_);
                        cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                        if (stop_ && tasks_.empty()) return;
                        task = std::move(tasks_.front());
                        tasks_.pop_front();
                    }
                    task();
                }
            });
        }
    }

    std::vector<std::jthread> workers_;
    std::deque<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
};

}  // namespace

std::future<Result<std::vector<uint8_t>>> AsyncIO::read_file(
    const std::filesystem::path& path) {
    return IoPool::instance().submit([path]() -> Result<std::vector<uint8_t>> {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
            return make_error(ErrorCode::IoFailed, "Failed to open file", path.string());
        }
        const auto size = file.tellg();
        file.seekg(0);
        std::vector<uint8_t> data(static_cast<size_t>(size));
        if (!file.read(reinterpret_cast<char*>(data.data()),
                       static_cast<std::streamsize>(size))) {
            return make_error(ErrorCode::IoFailed, "Failed to read file", path.string());
        }
        return data;
    });
}

std::future<Result<void>> AsyncIO::write_file(
    const std::filesystem::path& path,
    std::vector<uint8_t> data) {
    return IoPool::instance().submit(
        [path, data = std::move(data)]() -> Result<void> {
            // Ensure parent directory exists
            if (path.has_parent_path()) {
                std::error_code ec;
                std::filesystem::create_directories(path.parent_path(), ec);
                // ignore ec — directory may already exist
            }

            std::ofstream file(path, std::ios::binary);
            if (!file) {
                return make_error(ErrorCode::IoFailed, "Failed to create file", path.string());
            }
            if (!file.write(reinterpret_cast<const char*>(data.data()),
                            static_cast<std::streamsize>(data.size()))) {
                return make_error(ErrorCode::IoFailed, "Failed to write file", path.string());
            }
            return {};
        });
}

std::future<Result<std::filesystem::path>> AsyncIO::download_model(
    const std::string& url,
    const std::filesystem::path& cache_dir,
    const DownloadPolicy& policy) {
    return IoPool::instance().submit(
        [url, cache_dir, policy]() -> Result<std::filesystem::path> {
            // Extract filename from URL
            auto pos = url.rfind('/');
            std::string filename = (pos != std::string::npos) ? url.substr(pos + 1) : "model.onnx";
            auto cached_path = cache_dir / filename;

            // Check cache first
            if (std::filesystem::exists(cached_path)) {
                // TODO: verify checksum if policy.verify_checksum && !policy.expected_sha256.empty()
                return cached_path;
            }

            // Create cache directory
            std::error_code ec;
            std::filesystem::create_directories(cache_dir, ec);

            // Download with retry and exponential backoff.
            // For now, return an error — actual HTTP download requires libcurl
            // which will be added when segmentation node needs model loading.
            return make_error(
                ErrorCode::IoFailed,
                "HTTP download not yet implemented",
                "URL: " + url + " — place model file at " + cached_path.string());
        });
}

std::future<Result<std::string>> AsyncIO::load_yaml(
    const std::filesystem::path& path) {
    return IoPool::instance().submit([path]() -> Result<std::string> {
        std::ifstream file(path);
        if (!file) {
            return make_error(ErrorCode::IoFailed, "Failed to open YAML file", path.string());
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    });
}

}  // namespace iris
