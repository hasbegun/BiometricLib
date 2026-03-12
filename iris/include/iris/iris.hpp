#pragma once

// Core
#include <iris/core/algorithm.hpp>
#include <iris/core/async_io.hpp>
#include <iris/core/cancellation.hpp>
#include <iris/core/config.hpp>
#include <iris/core/error.hpp>
#include <iris/core/iris_code_packed.hpp>
#include <iris/core/node_registry.hpp>
#include <iris/core/pipeline_node.hpp>
#include <iris/core/thread_pool.hpp>
#include <iris/core/types.hpp>

// Nodes — Phase 2
#include <iris/nodes/bisectors_eye_center.hpp>
#include <iris/nodes/contour_extractor.hpp>
#include <iris/nodes/contour_interpolator.hpp>
#include <iris/nodes/contour_smoother.hpp>
#include <iris/nodes/ellipse_fitter.hpp>
#include <iris/nodes/encoder.hpp>
#include <iris/nodes/eyeball_distance_filter.hpp>
#include <iris/nodes/fusion_extrapolator.hpp>
#include <iris/nodes/linear_extrapolator.hpp>
#include <iris/nodes/matcher_simd.hpp>
#include <iris/nodes/moment_orientation.hpp>
#include <iris/nodes/onnx_segmentation.hpp>
#include <iris/nodes/segmentation_binarizer.hpp>
#include <iris/nodes/specular_reflection_detector.hpp>

// Nodes — Phase 3: Eye Properties
#include <iris/nodes/eccentricity_offgaze_estimation.hpp>
#include <iris/nodes/iris_bbox_calculator.hpp>
#include <iris/nodes/noise_mask_union.hpp>
#include <iris/nodes/occlusion_calculator.hpp>
#include <iris/nodes/pupil_iris_property_calculator.hpp>
#include <iris/nodes/sharpness_estimation.hpp>

// Nodes — Phase 3: Normalization
#include <iris/nodes/linear_normalization.hpp>
#include <iris/nodes/nonlinear_normalization.hpp>
#include <iris/nodes/perspective_normalization.hpp>

// Nodes — Phase 3: Validators
#include <iris/nodes/cross_object_validators.hpp>
#include <iris/nodes/object_validators.hpp>

// Nodes — Phase 4: Feature Extraction & Encoding
#include <iris/nodes/conv_filter_bank.hpp>
#include <iris/nodes/fragile_bit_refinement.hpp>
#include <iris/nodes/gabor_filter.hpp>
#include <iris/nodes/log_gabor_filter.hpp>
#include <iris/nodes/regular_probe_schema.hpp>

// Nodes — Phase 5: Matching & Template Operations
#include <iris/nodes/batch_matcher.hpp>
#include <iris/nodes/hamming_distance_matcher.hpp>
#include <iris/nodes/majority_vote_aggregation.hpp>
#include <iris/nodes/template_alignment.hpp>
#include <iris/nodes/template_identity_filter.hpp>

// Pipeline — Phase 6: Orchestration
#include <iris/pipeline/aggregation_pipeline.hpp>
#include <iris/pipeline/callback_dispatcher.hpp>
#include <iris/pipeline/iris_pipeline.hpp>
#include <iris/pipeline/multiframe_pipeline.hpp>
#include <iris/pipeline/node_dispatcher.hpp>
#include <iris/pipeline/pipeline_context.hpp>
#include <iris/pipeline/pipeline_dag.hpp>
#include <iris/pipeline/pipeline_executor.hpp>

// Utilities
#include <iris/utils/base64.hpp>
#include <iris/utils/filter_utils.hpp>
#include <iris/utils/geometry_utils.hpp>
#include <iris/utils/normalization_utils.hpp>
#include <iris/utils/template_serializer.hpp>
