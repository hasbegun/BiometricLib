#pragma once

#include <chrono>
#include <memory>

#include <eyetrack/core/cancellation.hpp>
#include <eyetrack/core/error.hpp>
#include <eyetrack/pipeline/eyetrack_pipeline.hpp>
#include <eyetrack/pipeline/pipeline_context.hpp>

namespace eyetrack {

/// Execution mode for the pipeline.
enum class ExecutionMode : uint8_t {
    Sync = 0,   // Single-threaded sequential execution
    Async = 1,  // Thread-pool based parallel execution
};

/// Pipeline execution engine.
///
/// Runs a configured EyetrackPipeline against a PipelineContext,
/// with optional frame-rate adaptation and cooperative cancellation.
class PipelineExecutor {
public:
    struct Config {
        ExecutionMode mode = ExecutionMode::Sync;
        uint32_t target_fps = 30;
    };

    PipelineExecutor() = default;
    explicit PipelineExecutor(const Config& config);

    /// Set the pipeline to execute.
    void set_pipeline(std::shared_ptr<EyetrackPipeline> pipeline);

    /// Execute the pipeline on the given context.
    /// Returns error if pipeline is empty, cancelled, or a node fails.
    [[nodiscard]] Result<void> execute(PipelineContext& context);

    /// Execute with cancellation support.
    [[nodiscard]] Result<void> execute(PipelineContext& context,
                                       CancellationToken& token);

    /// Check if the current frame should be skipped (frame-rate adaptation).
    /// Returns true if too little time has passed since the last frame.
    [[nodiscard]] bool should_skip_frame() const;

    /// Mark the start of a new frame (for frame-rate timing).
    void mark_frame_start();

    /// Get execution mode.
    [[nodiscard]] ExecutionMode mode() const noexcept { return config_.mode; }

    /// Get target FPS.
    [[nodiscard]] uint32_t target_fps() const noexcept {
        return config_.target_fps;
    }

private:
    Config config_;
    std::shared_ptr<EyetrackPipeline> pipeline_;
    std::chrono::steady_clock::time_point last_frame_time_{};
    bool has_last_frame_ = false;
};

}  // namespace eyetrack
