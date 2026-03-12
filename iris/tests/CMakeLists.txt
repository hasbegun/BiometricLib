include(GoogleTest)

# Helper to add a unit test
function(iris_add_test TEST_NAME)
    add_executable(${TEST_NAME} ${ARGN})
    target_link_libraries(${TEST_NAME} PRIVATE iris GTest::gtest_main)
    gtest_discover_tests(${TEST_NAME})
endfunction()

# --- Unit tests ---
iris_add_test(test_types unit/test_types.cpp)
iris_add_test(test_error unit/test_error.cpp)
iris_add_test(test_cancellation unit/test_cancellation.cpp)
iris_add_test(test_thread_pool unit/test_thread_pool.cpp)
iris_add_test(test_async_io unit/test_async_io.cpp)
iris_add_test(test_simd_popcount unit/test_simd_popcount.cpp)
iris_add_test(test_encoder unit/test_encoder.cpp)
iris_add_test(test_geometry_utils unit/test_geometry_utils.cpp)
iris_add_test(test_specular_reflection unit/test_specular_reflection.cpp)
iris_add_test(test_binarization unit/test_binarization.cpp)
iris_add_test(test_vectorization unit/test_vectorization.cpp)
iris_add_test(test_geometry_refinement unit/test_geometry_refinement.cpp)
iris_add_test(test_eye_centers unit/test_eye_centers.cpp)
iris_add_test(test_eye_orientation unit/test_eye_orientation.cpp)
iris_add_test(test_geometry_estimation unit/test_geometry_estimation.cpp)
iris_add_test(test_segmentation unit/test_segmentation.cpp)
iris_add_test(test_normalization_utils unit/test_normalization_utils.cpp)
iris_add_test(test_offgaze_estimation unit/test_offgaze_estimation.cpp)
iris_add_test(test_pupil_iris_property unit/test_pupil_iris_property.cpp)
iris_add_test(test_iris_bbox unit/test_iris_bbox.cpp)
iris_add_test(test_noise_mask_union unit/test_noise_mask_union.cpp)
iris_add_test(test_occlusion_calculator unit/test_occlusion_calculator.cpp)
iris_add_test(test_linear_normalization unit/test_linear_normalization.cpp)
iris_add_test(test_nonlinear_normalization unit/test_nonlinear_normalization.cpp)
iris_add_test(test_perspective_normalization unit/test_perspective_normalization.cpp)
iris_add_test(test_sharpness_estimation unit/test_sharpness_estimation.cpp)
iris_add_test(test_object_validators unit/test_object_validators.cpp)
iris_add_test(test_cross_object_validators unit/test_cross_object_validators.cpp)
iris_add_test(test_gabor_filter unit/test_gabor_filter.cpp)
iris_add_test(test_log_gabor_filter unit/test_log_gabor_filter.cpp)
iris_add_test(test_regular_probe_schema unit/test_regular_probe_schema.cpp)
iris_add_test(test_conv_filter_bank unit/test_conv_filter_bank.cpp)
iris_add_test(test_fragile_bit_refinement unit/test_fragile_bit_refinement.cpp)
iris_add_test(test_feature_extraction_chain unit/test_feature_extraction_chain.cpp)
iris_add_test(test_hamming_distance_matcher unit/test_hamming_distance_matcher.cpp)
iris_add_test(test_batch_matcher unit/test_batch_matcher.cpp)
iris_add_test(test_template_alignment unit/test_template_alignment.cpp)
iris_add_test(test_majority_vote_aggregation unit/test_majority_vote_aggregation.cpp)
iris_add_test(test_template_identity_filter unit/test_template_identity_filter.cpp)
iris_add_test(test_pipeline_context unit/test_pipeline_context.cpp)
iris_add_test(test_base64 unit/test_base64.cpp)
iris_add_test(test_template_serializer unit/test_template_serializer.cpp)
iris_add_test(test_npy_writer unit/test_npy_writer.cpp)
iris_add_test(test_secure_buffer unit/test_secure_buffer.cpp)
iris_add_test(test_template_npy unit/test_template_npy.cpp)
iris_add_test(test_api_hardening unit/test_api_hardening.cpp)

# test_node_dispatcher needs WHOLE_ARCHIVE for algorithm auto-registration
add_executable(test_node_dispatcher unit/test_node_dispatcher.cpp)
target_link_libraries(test_node_dispatcher PRIVATE
    $<LINK_LIBRARY:WHOLE_ARCHIVE,iris> GTest::gtest_main)
gtest_discover_tests(test_node_dispatcher)

# test_callback_dispatcher needs WHOLE_ARCHIVE for validator auto-registration
add_executable(test_callback_dispatcher unit/test_callback_dispatcher.cpp)
target_link_libraries(test_callback_dispatcher PRIVATE
    $<LINK_LIBRARY:WHOLE_ARCHIVE,iris> GTest::gtest_main)
gtest_discover_tests(test_callback_dispatcher)

# test_pipeline_dag needs WHOLE_ARCHIVE for algorithm auto-registration
add_executable(test_pipeline_dag unit/test_pipeline_dag.cpp)
target_link_libraries(test_pipeline_dag PRIVATE
    $<LINK_LIBRARY:WHOLE_ARCHIVE,iris> GTest::gtest_main)
gtest_discover_tests(test_pipeline_dag)

# test_pipeline_executor needs WHOLE_ARCHIVE for algorithm auto-registration
add_executable(test_pipeline_executor unit/test_pipeline_executor.cpp)
target_link_libraries(test_pipeline_executor PRIVATE
    $<LINK_LIBRARY:WHOLE_ARCHIVE,iris> GTest::gtest_main)
gtest_discover_tests(test_pipeline_executor)

# test_iris_pipeline needs WHOLE_ARCHIVE for algorithm auto-registration
add_executable(test_iris_pipeline unit/test_iris_pipeline.cpp)
target_link_libraries(test_iris_pipeline PRIVATE
    $<LINK_LIBRARY:WHOLE_ARCHIVE,iris> GTest::gtest_main)
gtest_discover_tests(test_iris_pipeline)

# test_multiframe_pipeline needs WHOLE_ARCHIVE for algorithm auto-registration
add_executable(test_multiframe_pipeline unit/test_multiframe_pipeline.cpp)
target_link_libraries(test_multiframe_pipeline PRIVATE
    $<LINK_LIBRARY:WHOLE_ARCHIVE,iris> GTest::gtest_main)
gtest_discover_tests(test_multiframe_pipeline)

# --- Integration tests ---

# test_pipeline_integration needs WHOLE_ARCHIVE for algorithm auto-registration
add_executable(test_pipeline_integration integration/test_pipeline_integration.cpp)
target_link_libraries(test_pipeline_integration PRIVATE
    $<LINK_LIBRARY:WHOLE_ARCHIVE,iris> GTest::gtest_main)
gtest_discover_tests(test_pipeline_integration)

# test_node_registry needs WHOLE_ARCHIVE to pull in static auto-registration
# objects (IRIS_REGISTER_NODE) that no test code directly references.
add_executable(test_node_registry unit/test_node_registry.cpp)
target_link_libraries(test_node_registry PRIVATE
    $<LINK_LIBRARY:WHOLE_ARCHIVE,iris> GTest::gtest_main)
gtest_discover_tests(test_node_registry)

# --- FHE tests (conditional on OpenFHE) ---
if(IRIS_ENABLE_FHE)
    iris_add_test(test_fhe_context unit/test_fhe_context.cpp)
    iris_add_test(test_encrypted_template unit/test_encrypted_template.cpp)
    iris_add_test(test_encrypted_matcher unit/test_encrypted_matcher.cpp)
    iris_add_test(test_key_manager unit/test_key_manager.cpp)
    iris_add_test(test_key_store unit/test_key_store.cpp)
    iris_add_test(test_fhe_integration integration/test_fhe_integration.cpp)
endif()

# --- Benchmarks ---
if(IRIS_ENABLE_BENCH)
    function(iris_add_bench BENCH_NAME)
        add_executable(${BENCH_NAME} ${ARGN})
        target_link_libraries(${BENCH_NAME} PRIVATE iris benchmark::benchmark_main)
    endfunction()

    iris_add_bench(bench_matcher bench/bench_matcher.cpp)

    if(IRIS_ENABLE_FHE)
        iris_add_bench(bench_crypto bench/bench_crypto.cpp)
    endif()
endif()
