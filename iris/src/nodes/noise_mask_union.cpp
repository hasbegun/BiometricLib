#include <iris/nodes/noise_mask_union.hpp>

#include <iris/core/node_registry.hpp>

namespace iris {

Result<NoiseMask> NoiseMaskUnion::run(
    const std::vector<NoiseMask>& elements) const {
    if (elements.empty()) {
        return make_error(ErrorCode::ValidationFailed,
                          "No noise masks to combine");
    }

    cv::Mat result = elements[0].mask.clone();
    for (size_t i = 1; i < elements.size(); ++i) {
        if (elements[i].mask.size() != result.size()) {
            return make_error(ErrorCode::ValidationFailed,
                              "Noise mask size mismatch");
        }
        cv::bitwise_or(result, elements[i].mask, result);
    }

    return NoiseMask{.mask = std::move(result)};
}

IRIS_REGISTER_NODE("iris.NoiseMaskUnion", NoiseMaskUnion);

}  // namespace iris
