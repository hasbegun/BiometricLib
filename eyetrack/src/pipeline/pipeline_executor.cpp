#include <eyetrack/pipeline/pipeline_executor.hpp>

namespace eyetrack {

PipelineExecutor::PipelineExecutor(const Config& config) : config_(config) {}

void PipelineExecutor::set_pipeline(
    std::shared_ptr<EyetrackPipeline> pipeline) {
    pipeline_ = std::move(pipeline);
}

Result<void> PipelineExecutor::execute(PipelineContext& context) {
    if (!pipeline_ || pipeline_->empty()) {
        return make_error(ErrorCode::ConfigInvalid,
                          "No pipeline configured or pipeline is empty");
    }

    for (const auto& node : pipeline_->nodes()) {
        auto result = node->execute(context.raw());
        if (!result.has_value()) {
            return result;
        }
    }

    return {};
}

Result<void> PipelineExecutor::execute(PipelineContext& context,
                                        CancellationToken& token) {
    if (!pipeline_ || pipeline_->empty()) {
        return make_error(ErrorCode::ConfigInvalid,
                          "No pipeline configured or pipeline is empty");
    }

    for (const auto& node : pipeline_->nodes()) {
        if (token.is_cancelled()) {
            return make_error(ErrorCode::Cancelled,
                              "Pipeline execution cancelled");
        }

        auto result = node->execute(context.raw());
        if (!result.has_value()) {
            return result;
        }
    }

    return {};
}

bool PipelineExecutor::should_skip_frame() const {
    if (!has_last_frame_ || config_.target_fps == 0) return false;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                       now - last_frame_time_)
                       .count();
    auto target_us =
        static_cast<long long>(1'000'000) /
        static_cast<long long>(config_.target_fps);

    return elapsed < target_us;
}

void PipelineExecutor::mark_frame_start() {
    last_frame_time_ = std::chrono::steady_clock::now();
    has_last_frame_ = true;
}

}  // namespace eyetrack
