#include <benchmark/benchmark.h>

#include <eyetrack/nodes/face_detector.hpp>
#include <opencv2/imgcodecs.hpp>

static const std::string kProjectRoot = EYETRACK_PROJECT_ROOT;
static const std::string kFixtureDir = kProjectRoot + "/tests/fixtures/face_images";
static const std::string kModelPath = kProjectRoot + "/models/face_landmark.onnx";

static void BM_FaceDetector_640x480(benchmark::State& state) {
    eyetrack::FaceDetector::Config config;
    config.model_path = kModelPath;
    config.confidence_threshold = 0.0F;
    eyetrack::FaceDetector detector(config);

    cv::Mat input = cv::imread(kFixtureDir + "/frontal_01_standard.png");
    if (input.empty()) {
        state.SkipWithError("Fixture image not found");
        return;
    }

    // Resize to 640x480 for consistent benchmarking
    cv::Mat resized;
    cv::resize(input, resized, cv::Size(640, 480));

    // Warm up: run once to trigger lazy model loading
    auto warmup = detector.run(resized);
    if (!warmup.has_value()) {
        state.SkipWithError("Face detection failed during warmup");
        return;
    }

    for (auto _ : state) {
        auto result = detector.run(resized);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_FaceDetector_640x480)->Unit(benchmark::kMillisecond);

static void BM_FaceDetector_Preprocess(benchmark::State& state) {
    cv::Mat input(480, 640, CV_8UC3);
    cv::randu(input, cv::Scalar::all(30), cv::Scalar::all(220));

    for (auto _ : state) {
        auto result = eyetrack::FaceDetector::preprocess(input);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_FaceDetector_Preprocess)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
