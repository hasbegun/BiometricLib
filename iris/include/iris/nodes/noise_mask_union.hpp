#pragma once

#include <string_view>
#include <unordered_map>
#include <vector>

#include <iris/core/algorithm.hpp>
#include <iris/core/error.hpp>
#include <iris/core/types.hpp>

namespace iris {

/// Bitwise OR union of multiple noise masks.
class NoiseMaskUnion : public Algorithm<NoiseMaskUnion> {
public:
    static constexpr std::string_view kName = "NoiseMaskUnion";

    NoiseMaskUnion() = default;
    explicit NoiseMaskUnion(
        const std::unordered_map<std::string, std::string>& /*node_params*/) {}

    Result<NoiseMask> run(const std::vector<NoiseMask>& elements) const;
};

}  // namespace iris
