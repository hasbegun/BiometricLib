#include <atomic>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <spdlog/spdlog.h>

#include <eyetrack/comm/eyetrack_server.hpp>
#include <eyetrack/core/config.hpp>

namespace {

std::atomic<bool> g_shutdown_requested{false};

void signal_handler(int /*sig*/) {
    g_shutdown_requested.store(true, std::memory_order_release);
}

}  // namespace

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::info);

    // Determine config path
    std::filesystem::path config_path = "config/server.yaml";
    if (argc > 1) {
        config_path = argv[1];  // NOLINT
    }

    spdlog::info("Loading config from {}", config_path.string());

    auto config_result = eyetrack::load_server_config(config_path);
    if (!config_result.has_value()) {
        spdlog::error("Failed to load config: {}", config_result.error().message);
        return 1;
    }

    auto config = config_result.value();

    // Set up signal handling
    std::signal(SIGINT, signal_handler);   // NOLINT
    std::signal(SIGTERM, signal_handler);  // NOLINT

    // Create and start the server
    eyetrack::EyetrackServer server(config);

    // Set a placeholder pipeline dispatcher
    server.set_dispatcher(
        [](const eyetrack::FrameData& frame)
            -> eyetrack::Result<eyetrack::GazeResult> {
            eyetrack::GazeResult gaze;
            gaze.gaze_point = {0.5F, 0.5F};
            gaze.confidence = 1.0F;
            gaze.blink = eyetrack::BlinkType::None;
            gaze.timestamp = frame.timestamp;
            return gaze;
        });

    auto start_result = server.start();
    if (!start_result.has_value()) {
        spdlog::error("Failed to start server: {}",
                      start_result.error().message);
        return 1;
    }

    spdlog::info("EyeTrack server running. Press Ctrl+C to stop.");

    // Wait for shutdown signal
    while (!g_shutdown_requested.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    spdlog::info("Shutdown signal received, stopping server...");
    server.stop();
    spdlog::info("Server stopped cleanly.");
    return 0;
}
