#include <iris/pipeline/callback_dispatcher.hpp>

#include <iris/nodes/cross_object_validators.hpp>
#include <iris/nodes/object_validators.hpp>

namespace iris {

// =============================================================================
// Template callback helpers
// =============================================================================

/// Single-argument validator callback.
template <typename Validator, typename OutputType>
Result<void> callback_1(const std::any& validator_any, const std::any& output_any) {
    const auto* validator = std::any_cast<Validator>(&validator_any);
    if (!validator) {
        return make_error(ErrorCode::ConfigInvalid,
                          "callback_1: wrong validator type for "
                              + std::string(typeid(Validator).name()));
    }
    const auto* output = std::any_cast<OutputType>(&output_any);
    if (!output) {
        return make_error(ErrorCode::ConfigInvalid,
                          "callback_1: wrong output type for "
                              + std::string(typeid(OutputType).name()));
    }
    auto result = validator->run(*output);
    if (!result) return std::unexpected(result.error());
    return {};
}

// =============================================================================
// Callback dispatch table
// =============================================================================

const std::unordered_map<std::string, CallbackDispatchFn>& callback_dispatch_table() {
    static const std::unordered_map<std::string, CallbackDispatchFn> table = {
        // Object validators (short and long-form names)
        {"iris.Pupil2IrisPropertyValidator",
         callback_1<Pupil2IrisPropertyValidator, PupilToIrisProperty>},
        {"iris.nodes.validators.object_validators.Pupil2IrisPropertyValidator",
         callback_1<Pupil2IrisPropertyValidator, PupilToIrisProperty>},

        {"iris.OffgazeValidator",
         callback_1<OffgazeValidator, Offgaze>},
        {"iris.nodes.validators.object_validators.OffgazeValidator",
         callback_1<OffgazeValidator, Offgaze>},

        {"iris.OcclusionValidator",
         callback_1<OcclusionValidator, EyeOcclusion>},
        {"iris.nodes.validators.object_validators.OcclusionValidator",
         callback_1<OcclusionValidator, EyeOcclusion>},

        {"iris.SharpnessValidator",
         callback_1<SharpnessValidator, Sharpness>},
        {"iris.nodes.validators.object_validators.SharpnessValidator",
         callback_1<SharpnessValidator, Sharpness>},

        {"iris.IsMaskTooSmallValidator",
         callback_1<IsMaskTooSmallValidator, IrisTemplate>},
        {"iris.nodes.validators.object_validators.IsMaskTooSmallValidator",
         callback_1<IsMaskTooSmallValidator, IrisTemplate>},

        {"iris.IsPupilInsideIrisValidator",
         callback_1<IsPupilInsideIrisValidator, GeometryPolygons>},
        {"iris.nodes.validators.object_validators.IsPupilInsideIrisValidator",
         callback_1<IsPupilInsideIrisValidator, GeometryPolygons>},

        {"iris.PolygonsLengthValidator",
         callback_1<PolygonsLengthValidator, GeometryPolygons>},
        {"iris.nodes.validators.object_validators.PolygonsLengthValidator",
         callback_1<PolygonsLengthValidator, GeometryPolygons>},
    };
    return table;
}

Result<CallbackDispatchFn> lookup_callback_dispatch(const std::string& class_name) {
    const auto& table = callback_dispatch_table();
    auto it = table.find(class_name);
    if (it == table.end()) {
        return make_error(
            ErrorCode::ConfigInvalid,
            "No callback dispatch function for class_name: " + class_name);
    }
    return it->second;
}

}  // namespace iris
