#include <cstdint>
#include <random>

#include <iris/core/iris_code_packed.hpp>
#include <iris/nodes/hamming_distance_matcher.hpp>

#include <benchmark/benchmark.h>

namespace iris::bench {

namespace {

PackedIrisCode random_code(std::mt19937_64& rng) {
    PackedIrisCode code;
    for (auto& w : code.code_bits) w = rng();
    for (auto& w : code.mask_bits) w = ~uint64_t{0};
    return code;
}

}  // namespace

static void BM_PlaintextMatch(benchmark::State& state) {
    std::mt19937_64 rng(42);
    auto probe = random_code(rng);
    auto gallery = random_code(rng);

    IrisTemplate probe_tmpl;
    probe_tmpl.iris_codes.push_back(probe);
    IrisTemplate gallery_tmpl;
    gallery_tmpl.iris_codes.push_back(gallery);

    SimpleHammingDistanceMatcher matcher;

    for (auto _ : state) {
        auto result = matcher.run(probe_tmpl, gallery_tmpl);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_PlaintextMatch);

static void BM_PlaintextMatchWithRotation(benchmark::State& state) {
    std::mt19937_64 rng(42);
    auto probe = random_code(rng);
    auto gallery = random_code(rng);

    IrisTemplate probe_tmpl;
    probe_tmpl.iris_codes.push_back(probe);
    IrisTemplate gallery_tmpl;
    gallery_tmpl.iris_codes.push_back(gallery);

    SimpleHammingDistanceMatcher::Params params;
    params.rotation_shift = 15;
    SimpleHammingDistanceMatcher matcher(params);

    for (auto _ : state) {
        auto result = matcher.run(probe_tmpl, gallery_tmpl);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_PlaintextMatchWithRotation);

}  // namespace iris::bench
