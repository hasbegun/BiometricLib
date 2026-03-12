#ifdef IRIS_HAS_FHE

#include <cstdint>
#include <random>

#include <iris/core/iris_code_packed.hpp>
#include <iris/crypto/encrypted_matcher.hpp>
#include <iris/crypto/encrypted_template.hpp>
#include <iris/crypto/fhe_context.hpp>

// Suppress warnings from OpenFHE
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wnull-dereference"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif

#include <benchmark/benchmark.h>

namespace iris::bench {

namespace {

iris::FHEContext& shared_ctx() {
    static auto ctx = [] {
        auto c = iris::FHEContext::create();
        c->generate_keys();
        return std::move(*c);
    }();
    return ctx;
}

PackedIrisCode random_code(std::mt19937_64& rng) {
    PackedIrisCode code;
    for (auto& w : code.code_bits) w = rng();
    for (auto& w : code.mask_bits) w = ~uint64_t{0};
    return code;
}

}  // namespace

static void BM_FHEKeyGeneration(benchmark::State& state) {
    for (auto _ : state) {
        auto ctx = iris::FHEContext::create();
        ctx->generate_keys();
        benchmark::DoNotOptimize(ctx);
    }
}
BENCHMARK(BM_FHEKeyGeneration)->Unit(benchmark::kMillisecond);

static void BM_FHEEncryption(benchmark::State& state) {
    auto& ctx = shared_ctx();
    std::mt19937_64 rng(42);
    auto code = random_code(rng);

    for (auto _ : state) {
        auto enc = EncryptedTemplate::encrypt(ctx, code);
        benchmark::DoNotOptimize(enc);
    }
}
BENCHMARK(BM_FHEEncryption)->Unit(benchmark::kMillisecond);

static void BM_FHEEncryptedMatch(benchmark::State& state) {
    auto& ctx = shared_ctx();
    std::mt19937_64 rng(42);
    auto probe = random_code(rng);
    auto gallery = random_code(rng);

    auto enc_probe = EncryptedTemplate::encrypt(ctx, probe);
    auto enc_gallery = EncryptedTemplate::encrypt(ctx, gallery);

    for (auto _ : state) {
        auto result = EncryptedMatcher::match_encrypted(ctx, *enc_probe, *enc_gallery);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_FHEEncryptedMatch)->Unit(benchmark::kMillisecond);

static void BM_FHEFullMatch(benchmark::State& state) {
    auto& ctx = shared_ctx();
    std::mt19937_64 rng(42);
    auto probe = random_code(rng);
    auto gallery = random_code(rng);

    for (auto _ : state) {
        auto hd = EncryptedMatcher::match(ctx, probe, gallery);
        benchmark::DoNotOptimize(hd);
    }
}
BENCHMARK(BM_FHEFullMatch)->Unit(benchmark::kMillisecond);

}  // namespace iris::bench

#pragma GCC diagnostic pop

#endif  // IRIS_HAS_FHE
