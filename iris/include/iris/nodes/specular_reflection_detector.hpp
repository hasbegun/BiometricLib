#pragma once

#include <string_view>
#include <unordered_map>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Detect specular reflections in an infrared iris image.
///
/// Applies a simple intensity threshold — pixels above the threshold
/// are marked as reflection noise (value 1 in the output mask).
class SpecularReflectionDetector : public Algorithm<SpecularReflectionDetector> {
public:
    static constexpr std::string_view kName = "SpecularReflectionDetector";

    struct Params {
        int reflection_threshold = 254;
    };

    SpecularReflectionDetector() = default;
    explicit SpecularReflectionDetector(const Params& params) : params_(params) {}
    explicit SpecularReflectionDetector(
        const std::unordered_map<std::string, std::string>& node_params);

    Result<NoiseMask> run(const IRImage& image) const;

private:
    Params params_;
};

}  // namespace iris
