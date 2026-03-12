cmake_minimum_required(VERSION 3.28)
project(iris VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Options ---
option(IRIS_ENABLE_FHE "Build with OpenFHE support" OFF)
option(IRIS_ENABLE_TESTS "Build unit tests" ON)
option(IRIS_ENABLE_BENCH "Build benchmarks" OFF)
option(IRIS_ENABLE_SANITIZERS "Enable ASan + UBSan" OFF)
option(IRIS_ENABLE_TSAN "Enable ThreadSanitizer (mutually exclusive with ASan)" OFF)
option(IRIS_ENABLE_COMPARISON "Build comparison tool for cross-language validation" OFF)

# --- Compiler warnings ---
add_library(iris_warnings INTERFACE)
target_compile_options(iris_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Werror
    -Wno-unused-parameter  # allow during development
    -Wconversion -Wsign-conversion
    -Wdouble-promotion
    -Wnull-dereference
    -Wformat=2
)

# --- Sanitizers ---
if(IRIS_ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

if(IRIS_ENABLE_TSAN)
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
    add_link_options(-fsanitize=thread)
endif()

# --- Dependencies ---
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
find_package(yaml-cpp REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Eigen3 REQUIRED)

# ONNX Runtime — find via cmake or manual path
find_library(ONNXRUNTIME_LIB onnxruntime HINTS ${ONNXRUNTIME_ROOT}/lib)
find_path(ONNXRUNTIME_INCLUDE onnxruntime_cxx_api.h
    HINTS ${ONNXRUNTIME_ROOT}/include
    PATH_SUFFIXES onnxruntime/core/session)
if(NOT ONNXRUNTIME_LIB OR NOT ONNXRUNTIME_INCLUDE)
    message(WARNING "ONNX Runtime not found. Segmentation node will not build.")
    set(IRIS_HAS_ONNXRUNTIME OFF)
else()
    set(IRIS_HAS_ONNXRUNTIME ON)
endif()

# OpenFHE (optional)
if(IRIS_ENABLE_FHE)
    find_package(OpenFHE REQUIRED)
endif()

# --- Main library ---
file(GLOB_RECURSE IRIS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pipeline/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/*.cpp
)

if(IRIS_ENABLE_FHE)
    file(GLOB_RECURSE IRIS_CRYPTO_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/crypto/*.cpp)
    list(APPEND IRIS_SOURCES ${IRIS_CRYPTO_SOURCES})
endif()

add_library(iris ${IRIS_SOURCES})
target_include_directories(iris PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(iris PUBLIC
    iris_warnings
    opencv_core opencv_imgproc opencv_imgcodecs
    yaml-cpp::yaml-cpp
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    Eigen3::Eigen
)

if(IRIS_HAS_ONNXRUNTIME)
    target_include_directories(iris SYSTEM PUBLIC ${ONNXRUNTIME_INCLUDE})
    target_link_libraries(iris PUBLIC ${ONNXRUNTIME_LIB})
    target_compile_definitions(iris PUBLIC IRIS_HAS_ONNXRUNTIME)
endif()

if(IRIS_ENABLE_FHE)
    # OpenFHE targets don't set INTERFACE_INCLUDE_DIRECTORIES, so add manually.
    # Use SYSTEM to suppress warnings from OpenFHE headers under -Werror.
    target_include_directories(iris SYSTEM PUBLIC ${OpenFHE_INCLUDE})
    target_include_directories(iris SYSTEM PUBLIC ${OpenFHE_INCLUDE}/core)
    target_include_directories(iris SYSTEM PUBLIC ${OpenFHE_INCLUDE}/pke)
    target_include_directories(iris SYSTEM PUBLIC ${OpenFHE_INCLUDE}/binfhe)
    target_include_directories(iris SYSTEM PUBLIC ${OPENMP_INCLUDES})
    target_link_libraries(iris PUBLIC ${OpenFHE_LIBRARIES})
    target_compile_definitions(iris PUBLIC IRIS_HAS_FHE)

    # GCC 13 emits false-positive -Wnull-dereference from cereal template
    # instantiations (used by OpenFHE serialization) during inlining at -O3.
    # Pragmas cannot suppress this; disable the warning per-file instead.
    file(GLOB_RECURSE IRIS_CRYPTO_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/crypto/*.cpp)
    set_source_files_properties(${IRIS_CRYPTO_SRC}
        PROPERTIES COMPILE_OPTIONS "-Wno-null-dereference")
endif()

# --- Platform SIMD flags ---
# Target platform: Linux aarch64 (ARM64). NEON is always available — no extra flags needed.
# x86 SIMD detection retained below for build compatibility only (not a target).
include(CheckCXXCompilerFlag)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    check_cxx_compiler_flag("-mavx2" HAS_AVX2)
    if(HAS_AVX2)
        target_compile_options(iris PRIVATE -mavx2)
    endif()
    check_cxx_compiler_flag("-mavx512vpopcntdq" HAS_AVX512VPOPCNT)
    if(HAS_AVX512VPOPCNT)
        target_compile_options(iris PRIVATE -mavx512vpopcntdq -mavx512f)
    endif()
endif()

# --- Benchmarks ---
if(IRIS_ENABLE_BENCH)
    find_package(benchmark REQUIRED)
endif()

# --- Comparison tool ---
if(IRIS_ENABLE_COMPARISON)
    add_executable(iris_comparison comparison/iris_comparison.cpp)
    target_include_directories(iris_comparison PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    target_link_libraries(iris_comparison PRIVATE
        -Wl,--whole-archive iris -Wl,--no-whole-archive)

    # FHE comparison tool (compares plaintext vs encrypted HD scores)
    add_executable(fhe_comparison comparison/fhe_comparison.cpp)
    target_include_directories(fhe_comparison PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    target_link_libraries(fhe_comparison PRIVATE
        -Wl,--whole-archive iris -Wl,--no-whole-archive)
endif()

# --- Tests ---
if(IRIS_ENABLE_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    add_subdirectory(tests)
endif()
