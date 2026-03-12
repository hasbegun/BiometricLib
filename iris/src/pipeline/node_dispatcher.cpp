#include <iris/pipeline/node_dispatcher.hpp>

#include <sstream>

// All algorithm node headers — needed for std::any_cast in dispatch functions.
#include <iris/nodes/bisectors_eye_center.hpp>
#include <iris/nodes/contour_extractor.hpp>
#include <iris/nodes/contour_interpolator.hpp>
#include <iris/nodes/contour_smoother.hpp>
#include <iris/nodes/conv_filter_bank.hpp>
#include <iris/nodes/cross_object_validators.hpp>
#include <iris/nodes/eccentricity_offgaze_estimation.hpp>
#include <iris/nodes/encoder.hpp>
#include <iris/nodes/eyeball_distance_filter.hpp>
#include <iris/nodes/fragile_bit_refinement.hpp>
#include <iris/nodes/fusion_extrapolator.hpp>
#include <iris/nodes/iris_bbox_calculator.hpp>
#include <iris/nodes/linear_normalization.hpp>
#include <iris/nodes/moment_orientation.hpp>
#include <iris/nodes/noise_mask_union.hpp>
#include <iris/nodes/object_validators.hpp>
#include <iris/nodes/occlusion_calculator.hpp>
#include <iris/nodes/onnx_segmentation.hpp>
#include <iris/nodes/pupil_iris_property_calculator.hpp>
#include <iris/nodes/segmentation_binarizer.hpp>
#include <iris/nodes/sharpness_estimation.hpp>
#include <iris/nodes/specular_reflection_detector.hpp>

namespace iris {

// =============================================================================
// Template dispatch helpers
// =============================================================================

/// 1-input, 1-output dispatch.
template <typename Algo, typename In, typename Out>
Result<void> dispatch_1(
    const std::any& algo_any,
    const NodeConfig& config,
    PipelineContext& ctx) {
    auto in_r = read_input<In>(ctx, config.inputs[0]);
    if (!in_r) return std::unexpected(in_r.error());
    auto result = std::any_cast<const Algo&>(algo_any).run(**in_r);
    if (!result) return std::unexpected(result.error());
    ctx.set(config.name, std::move(*result));
    return {};
}

/// 1-input, pair output dispatch.
/// Stores elements under "name:0" and "name:1".
template <typename Algo, typename In, typename Out1, typename Out2>
Result<void> dispatch_1_pair(
    const std::any& algo_any,
    const NodeConfig& config,
    PipelineContext& ctx) {
    auto in_r = read_input<In>(ctx, config.inputs[0]);
    if (!in_r) return std::unexpected(in_r.error());
    auto result = std::any_cast<const Algo&>(algo_any).run(**in_r);
    if (!result) return std::unexpected(result.error());
    ctx.set(config.name + ":0", std::move(result->first));
    ctx.set(config.name + ":1", std::move(result->second));
    return {};
}

/// 2-input, 1-output dispatch.
template <typename Algo, typename In1, typename In2, typename Out>
Result<void> dispatch_2(
    const std::any& algo_any,
    const NodeConfig& config,
    PipelineContext& ctx) {
    auto in1 = read_input<In1>(ctx, config.inputs[0]);
    if (!in1) return std::unexpected(in1.error());
    auto in2 = read_input<In2>(ctx, config.inputs[1]);
    if (!in2) return std::unexpected(in2.error());
    auto result = std::any_cast<const Algo&>(algo_any).run(**in1, **in2);
    if (!result) return std::unexpected(result.error());
    ctx.set(config.name, std::move(*result));
    return {};
}

/// 2-input validator dispatch (void output — just validates).
template <typename Algo, typename In1, typename In2>
Result<void> dispatch_2_void(
    const std::any& algo_any,
    const NodeConfig& config,
    PipelineContext& ctx) {
    auto in1 = read_input<In1>(ctx, config.inputs[0]);
    if (!in1) return std::unexpected(in1.error());
    auto in2 = read_input<In2>(ctx, config.inputs[1]);
    if (!in2) return std::unexpected(in2.error());
    auto result = std::any_cast<const Algo&>(algo_any).run(**in1, **in2);
    if (!result) return std::unexpected(result.error());
    // Validators return std::monostate — nothing to store.
    return {};
}

/// 4-input, 1-output dispatch.
template <typename Algo, typename In1, typename In2, typename In3, typename In4, typename Out>
Result<void> dispatch_4(
    const std::any& algo_any,
    const NodeConfig& config,
    PipelineContext& ctx) {
    auto in1 = read_input<In1>(ctx, config.inputs[0]);
    if (!in1) return std::unexpected(in1.error());
    auto in2 = read_input<In2>(ctx, config.inputs[1]);
    if (!in2) return std::unexpected(in2.error());
    auto in3 = read_input<In3>(ctx, config.inputs[2]);
    if (!in3) return std::unexpected(in3.error());
    auto in4 = read_input<In4>(ctx, config.inputs[3]);
    if (!in4) return std::unexpected(in4.error());
    auto result = std::any_cast<const Algo&>(algo_any).run(**in1, **in2, **in3, **in4);
    if (!result) return std::unexpected(result.error());
    ctx.set(config.name, std::move(*result));
    return {};
}

// =============================================================================
// Custom dispatch functions
// =============================================================================

/// OcclusionCalculator: YAML order [noise_mask, polygons, orientation, centers]
/// differs from run() order [polygons, noise_mask, orientation, centers].
Result<void> dispatch_occlusion(
    const std::any& algo_any,
    const NodeConfig& config,
    PipelineContext& ctx) {
    // YAML: inputs[0]=noise_mask, inputs[1]=extrapolated_polygons,
    //       inputs[2]=eye_orientation, inputs[3]=eye_centers
    auto noise = read_input<NoiseMask>(ctx, config.inputs[0]);
    if (!noise) return std::unexpected(noise.error());
    auto polys = read_input<GeometryPolygons>(ctx, config.inputs[1]);
    if (!polys) return std::unexpected(polys.error());
    auto orient = read_input<EyeOrientation>(ctx, config.inputs[2]);
    if (!orient) return std::unexpected(orient.error());
    auto centers = read_input<EyeCenters>(ctx, config.inputs[3]);
    if (!centers) return std::unexpected(centers.error());

    // run() signature: (GeometryPolygons, NoiseMask, EyeOrientation, EyeCenters)
    auto result = std::any_cast<const OcclusionCalculator&>(algo_any)
                      .run(**polys, **noise, **orient, **centers);
    if (!result) return std::unexpected(result.error());
    ctx.set(config.name, std::move(*result));
    return {};
}

/// NoiseMaskUnion: multi-source input stored as comma-separated "name:index" pairs.
Result<void> dispatch_noise_union(
    const std::any& algo_any,
    const NodeConfig& config,
    PipelineContext& ctx) {
    const auto& src = config.inputs[0].source_node;

    // Parse "segmentation_binarization:1,specular_reflection_detection"
    std::vector<NoiseMask> masks;
    std::istringstream stream(src);
    std::string token;
    while (std::getline(stream, token, ',')) {
        auto colon = token.find(':');
        if (colon != std::string::npos) {
            // "name:index" — read indexed element
            std::string name = token.substr(0, colon);
            std::string idx_key = name + ":" + token.substr(colon + 1);
            auto r = ctx.get<NoiseMask>(idx_key);
            if (!r) return std::unexpected(r.error());
            masks.push_back(**r);
        } else {
            auto r = ctx.get<NoiseMask>(token);
            if (!r) return std::unexpected(r.error());
            masks.push_back(**r);
        }
    }

    auto result = std::any_cast<const NoiseMaskUnion&>(algo_any).run(masks);
    if (!result) return std::unexpected(result.error());
    ctx.set(config.name, std::move(*result));
    return {};
}

// =============================================================================
// Dispatch table
// =============================================================================

const std::unordered_map<std::string, DispatchFn>& dispatch_table() {
    static const std::unordered_map<std::string, DispatchFn> table = {
        // Segmentation
        {"iris.MultilabelSegmentation.create_from_hugging_face",
         dispatch_1<OnnxSegmentation, IRImage, SegmentationMap>},

        // Binarization (pair output)
        {"iris.MultilabelSegmentationBinarization",
         dispatch_1_pair<SegmentationBinarizer, SegmentationMap, GeometryMask, NoiseMask>},

        // Vectorization (reads indexed input from pair)
        {"iris.ContouringAlgorithm",
         dispatch_1<ContourExtractor, GeometryMask, GeometryPolygons>},

        // Specular reflection
        {"iris.SpecularReflectionDetection",
         dispatch_1<SpecularReflectionDetector, IRImage, NoiseMask>},

        // Contour interpolation
        {"iris.ContourInterpolation",
         dispatch_1<ContourInterpolator, GeometryPolygons, GeometryPolygons>},

        // Distance filter (2 inputs — indexed NoiseMask handled by read_input)
        {"iris.ContourPointNoiseEyeballDistanceFilter",
         dispatch_2<EyeballDistanceFilter, GeometryPolygons, NoiseMask, GeometryPolygons>},

        // Eye orientation
        {"iris.MomentOfArea",
         dispatch_1<MomentOrientation, GeometryPolygons, EyeOrientation>},

        // Eye centers
        {"iris.BisectorsMethod",
         dispatch_1<BisectorsEyeCenter, GeometryPolygons, EyeCenters>},

        // Smoothing
        {"iris.Smoothing",
         dispatch_2<ContourSmoother, GeometryPolygons, EyeCenters, GeometryPolygons>},

        // Geometry estimation (fusion extrapolation)
        {"iris.FusionExtrapolation",
         dispatch_2<FusionExtrapolator, GeometryPolygons, EyeCenters, GeometryPolygons>},

        // Pupil-iris property
        {"iris.PupilIrisPropertyCalculator",
         dispatch_2<PupilIrisPropertyCalculator, GeometryPolygons, EyeCenters, PupilToIrisProperty>},

        // Offgaze estimation
        {"iris.EccentricityOffgazeEstimation",
         dispatch_1<EccentricityOffgazeEstimation, GeometryPolygons, Offgaze>},

        // Occlusion (custom — YAML arg order != run() order)
        {"iris.OcclusionCalculator", dispatch_occlusion},

        // Noise mask aggregation (custom — multi-source input)
        {"iris.NoiseMaskUnion", dispatch_noise_union},

        // Linear normalization (4 inputs)
        {"iris.LinearNormalization",
         dispatch_4<LinearNormalization, IRImage, NoiseMask, GeometryPolygons,
                    EyeOrientation, NormalizedIris>},

        // Sharpness estimation
        {"iris.SharpnessEstimation",
         dispatch_1<SharpnessEstimation, NormalizedIris, Sharpness>},

        // Filter bank
        {"iris.ConvFilterBank",
         dispatch_1<ConvFilterBank, NormalizedIris, IrisFilterResponse>},

        // Fragile bit refinement (both short and long-form names)
        {"iris.FragileBitRefinement",
         dispatch_1<FragileBitRefinement, IrisFilterResponse, IrisFilterResponse>},
        {"iris.nodes.iris_response_refinement.fragile_bits_refinement.FragileBitRefinement",
         dispatch_1<FragileBitRefinement, IrisFilterResponse, IrisFilterResponse>},

        // Encoder
        {"iris.IrisEncoder",
         dispatch_1<IrisEncoder, IrisFilterResponse, IrisTemplate>},

        // Bounding box
        {"iris.IrisBBoxCalculator",
         dispatch_2<IrisBBoxCalculator, IRImage, GeometryPolygons, BoundingBox>},

        // Validators as standalone pipeline nodes (long-form names from YAML)
        {"iris.nodes.validators.cross_object_validators.EyeCentersInsideImageValidator",
         dispatch_2_void<EyeCentersInsideImageValidator, IRImage, EyeCenters>},
        {"iris.EyeCentersInsideImageValidator",
         dispatch_2_void<EyeCentersInsideImageValidator, IRImage, EyeCenters>},
        {"iris.nodes.validators.cross_object_validators.ExtrapolatedPolygonsInsideImageValidator",
         dispatch_2_void<ExtrapolatedPolygonsInsideImageValidator, IRImage, GeometryPolygons>},
        {"iris.ExtrapolatedPolygonsInsideImageValidator",
         dispatch_2_void<ExtrapolatedPolygonsInsideImageValidator, IRImage, GeometryPolygons>},
    };
    return table;
}

Result<DispatchFn> lookup_dispatch(const std::string& class_name) {
    const auto& table = dispatch_table();
    auto it = table.find(class_name);
    if (it == table.end()) {
        return make_error(
            ErrorCode::ConfigInvalid,
            "No dispatch function for class_name: " + class_name);
    }
    return it->second;
}

}  // namespace iris
