cmake_minimum_required(VERSION 3.25)
project(eyetrack VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Options ---
option(EYETRACK_ENABLE_TESTS "Build unit tests" ON)
option(EYETRACK_ENABLE_BENCH "Build benchmarks" OFF)
option(EYETRACK_ENABLE_SANITIZERS "Enable ASan + UBSan" OFF)
option(EYETRACK_ENABLE_TSAN "Enable ThreadSanitizer (mutually exclusive with ASan)" OFF)
option(EYETRACK_ENABLE_GRPC "Build with gRPC support" OFF)
option(EYETRACK_ENABLE_MQTT "Build with MQTT support" OFF)

# --- Compiler warnings ---
add_library(eyetrack_warnings INTERFACE)
target_compile_options(eyetrack_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Werror
    -Wno-unused-parameter
    -Wconversion -Wsign-conversion
    -Wdouble-promotion
    -Wformat=2
)
# GCC 12 (bookworm) emits false-positive -Wnull-dereference from libstdc++
# istreambuf_iterator templates. Only enable on GCC >= 13.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "13")
    target_compile_options(eyetrack_warnings INTERFACE -Wnull-dereference)
endif()

# --- Sanitizers ---
if(EYETRACK_ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

if(EYETRACK_ENABLE_TSAN)
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
    add_link_options(-fsanitize=thread)
endif()

# --- Dependencies ---
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
find_package(yaml-cpp REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Eigen3 REQUIRED)

# ONNX Runtime
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_library(ONNXRUNTIME_LIB onnxruntime HINTS ${ONNXRUNTIME_ROOT}/lib)
find_path(ONNXRUNTIME_INCLUDE onnxruntime_cxx_api.h
    HINTS ${ONNXRUNTIME_ROOT}/include
    PATH_SUFFIXES onnxruntime/core/session)
if(NOT ONNXRUNTIME_LIB OR NOT ONNXRUNTIME_INCLUDE)
    message(WARNING "ONNX Runtime not found. Face detection node will use stub fallback.")
    set(EYETRACK_HAS_ONNXRUNTIME OFF)
else()
    set(EYETRACK_HAS_ONNXRUNTIME ON)
endif()

# dlib (optional)
find_package(dlib QUIET)
if(dlib_FOUND)
    set(EYETRACK_HAS_DLIB ON)
    message(STATUS "dlib found: face detector dlib backend enabled.")
else()
    set(EYETRACK_HAS_DLIB OFF)
    message(STATUS "dlib not found. dlib face detector backend disabled.")
endif()

# Protobuf + gRPC (optional)
if(EYETRACK_ENABLE_GRPC)
    find_package(Protobuf REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GRPC REQUIRED grpc++ grpc)

    # Find the grpc_cpp_plugin for code generation
    find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)
    if(NOT GRPC_CPP_PLUGIN)
        message(FATAL_ERROR "grpc_cpp_plugin not found")
    endif()

    # Generate C++ from .proto files (protobuf + gRPC stubs)
    set(PROTO_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
    set(PROTO_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto_gen)
    file(MAKE_DIRECTORY ${PROTO_OUT_DIR})

    set(PROTO_FILE ${PROTO_SRC_DIR}/eyetrack.proto)
    set(PROTO_GEN_CC ${PROTO_OUT_DIR}/eyetrack.pb.cc)
    set(PROTO_GEN_H  ${PROTO_OUT_DIR}/eyetrack.pb.h)
    set(GRPC_GEN_CC  ${PROTO_OUT_DIR}/eyetrack.grpc.pb.cc)
    set(GRPC_GEN_H   ${PROTO_OUT_DIR}/eyetrack.grpc.pb.h)

    add_custom_command(
        OUTPUT ${PROTO_GEN_CC} ${PROTO_GEN_H}
        COMMAND protobuf::protoc
            --cpp_out=${PROTO_OUT_DIR}
            -I${PROTO_SRC_DIR}
            ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating protobuf C++ sources"
    )

    add_custom_command(
        OUTPUT ${GRPC_GEN_CC} ${GRPC_GEN_H}
        COMMAND protobuf::protoc
            --grpc_out=${PROTO_OUT_DIR}
            --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
            -I${PROTO_SRC_DIR}
            ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating gRPC C++ stubs"
    )

    add_library(eyetrack_proto ${PROTO_GEN_CC} ${GRPC_GEN_CC})
    target_include_directories(eyetrack_proto PUBLIC ${PROTO_OUT_DIR})
    target_link_libraries(eyetrack_proto PUBLIC
        protobuf::libprotobuf
        ${GRPC_LIBRARIES}
    )
    target_include_directories(eyetrack_proto SYSTEM PUBLIC ${GRPC_INCLUDE_DIRS})
    # Suppress warnings in generated code
    target_compile_options(eyetrack_proto PRIVATE -w)

    set(EYETRACK_HAS_GRPC ON)
endif()

# MQTT (optional)
if(EYETRACK_ENABLE_MQTT)
    find_library(PAHO_MQTT_CPP_LIB paho-mqttpp3 HINTS ${PAHO_MQTT_ROOT}/lib)
    find_library(PAHO_MQTT_C_LIB paho-mqtt3as HINTS ${PAHO_MQTT_ROOT}/lib)
    find_path(PAHO_MQTT_CPP_INCLUDE mqtt/async_client.h
        HINTS ${PAHO_MQTT_ROOT}/include)
    if(NOT PAHO_MQTT_CPP_LIB OR NOT PAHO_MQTT_C_LIB)
        message(WARNING "Paho MQTT C++ not found. MQTT handler will not build.")
        set(EYETRACK_HAS_MQTT OFF)
    else()
        set(EYETRACK_HAS_MQTT ON)
    endif()
endif()

# --- Project root define (for tests) ---
add_compile_definitions(EYETRACK_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")

# --- Main library ---
file(GLOB_RECURSE EYETRACK_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pipeline/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/comm/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/*.cpp
)

# If no source files exist yet, create a placeholder object library
if(EYETRACK_SOURCES)
    add_library(eyetrack ${EYETRACK_SOURCES})
else()
    # Placeholder: empty INTERFACE library until source files are added
    add_library(eyetrack INTERFACE)
    set(EYETRACK_LIB_IS_INTERFACE TRUE)
endif()

if(EYETRACK_LIB_IS_INTERFACE)
    target_include_directories(eyetrack INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(eyetrack INTERFACE
        eyetrack_warnings
        opencv_core opencv_imgproc opencv_imgcodecs
        yaml-cpp
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Eigen3::Eigen
    )
else()
    target_include_directories(eyetrack PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(eyetrack PUBLIC
        eyetrack_warnings
        opencv_core opencv_imgproc opencv_imgcodecs
        yaml-cpp
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Eigen3::Eigen
    )
endif()

if(EYETRACK_HAS_ONNXRUNTIME AND NOT EYETRACK_LIB_IS_INTERFACE)
    target_include_directories(eyetrack SYSTEM PUBLIC ${ONNXRUNTIME_INCLUDE})
    target_link_libraries(eyetrack PUBLIC ${ONNXRUNTIME_LIB})
    target_compile_definitions(eyetrack PUBLIC EYETRACK_HAS_ONNXRUNTIME)
endif()

if(EYETRACK_HAS_DLIB AND NOT EYETRACK_LIB_IS_INTERFACE)
    target_link_libraries(eyetrack PUBLIC dlib::dlib)
    target_compile_definitions(eyetrack PUBLIC EYETRACK_HAS_DLIB)
endif()

if(EYETRACK_HAS_GRPC AND NOT EYETRACK_LIB_IS_INTERFACE)
    target_link_libraries(eyetrack PUBLIC eyetrack_proto)
    target_compile_definitions(eyetrack PUBLIC EYETRACK_HAS_GRPC)
endif()

if(EYETRACK_HAS_MQTT AND NOT EYETRACK_LIB_IS_INTERFACE)
    target_include_directories(eyetrack SYSTEM PUBLIC ${PAHO_MQTT_CPP_INCLUDE})
    target_link_libraries(eyetrack PUBLIC ${PAHO_MQTT_CPP_LIB} ${PAHO_MQTT_C_LIB})
    target_compile_definitions(eyetrack PUBLIC EYETRACK_HAS_MQTT)
endif()

# Boost.Beast (WebSocket) + OpenSSL (TLS)
if(NOT EYETRACK_LIB_IS_INTERFACE)
    find_package(Boost QUIET COMPONENTS system)
    find_package(OpenSSL QUIET)
    if(Boost_FOUND AND OpenSSL_FOUND)
        target_link_libraries(eyetrack PUBLIC
            Boost::system OpenSSL::SSL OpenSSL::Crypto)
        target_compile_definitions(eyetrack PUBLIC EYETRACK_HAS_WEBSOCKET)
        message(STATUS "Boost + OpenSSL found: WebSocket support enabled.")
    else()
        message(STATUS "Boost or OpenSSL not found. WebSocket support disabled.")
    endif()
endif()

# --- Platform SIMD flags ---
include(CheckCXXCompilerFlag)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    check_cxx_compiler_flag("-mavx2" HAS_AVX2)
    if(HAS_AVX2 AND NOT EYETRACK_LIB_IS_INTERFACE)
        target_compile_options(eyetrack PRIVATE -mavx2)
    endif()
endif()

# --- Benchmarks ---
if(EYETRACK_ENABLE_BENCH)
    find_package(benchmark REQUIRED)
endif()

# --- Server executable ---
if(NOT EYETRACK_LIB_IS_INTERFACE)
    add_executable(eyetrack_server src/main.cpp)
    target_link_libraries(eyetrack_server PRIVATE
        $<LINK_LIBRARY:WHOLE_ARCHIVE,eyetrack>)
endif()

# --- Tools ---
if(EYETRACK_ENABLE_TESTS AND NOT EYETRACK_LIB_IS_INTERFACE)
    add_executable(generate_eye_fixtures tools/generate_eye_fixtures.cpp)
    target_link_libraries(generate_eye_fixtures PRIVATE
        $<LINK_LIBRARY:WHOLE_ARCHIVE,eyetrack>)
endif()

# --- FFI shared library ---
option(EYETRACK_BUILD_FFI "Build shared library for dart:ffi" OFF)

if(EYETRACK_BUILD_FFI AND NOT EYETRACK_LIB_IS_INTERFACE)
    add_library(eyetrack_ffi SHARED src/ffi/eyetrack_ffi.cpp)
    target_link_libraries(eyetrack_ffi PRIVATE eyetrack)
    target_include_directories(eyetrack_ffi PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )
    set_target_properties(eyetrack_ffi PROPERTIES
        OUTPUT_NAME "eyetrack_ffi"
        INSTALL_RPATH "@executable_path/../Frameworks"
        BUILD_WITH_INSTALL_RPATH TRUE
        MACOSX_RPATH TRUE
    )
endif()

# --- Tests ---
if(EYETRACK_ENABLE_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    add_subdirectory(tests)
endif()
