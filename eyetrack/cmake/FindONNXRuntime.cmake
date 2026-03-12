# FindONNXRuntime.cmake
#
# Finds the ONNX Runtime library.
#
# Sets:
#   ONNXRUNTIME_FOUND       - TRUE if found
#   ONNXRUNTIME_INCLUDE_DIR - Include directory
#   ONNXRUNTIME_LIBRARY     - Library path
#   ONNXRuntime::ONNXRuntime - Imported target
#
# Hints:
#   ONNXRUNTIME_ROOT - Root directory of ONNX Runtime installation

find_path(ONNXRUNTIME_INCLUDE_DIR
    NAMES onnxruntime_cxx_api.h
    HINTS
        ${ONNXRUNTIME_ROOT}/include
        /usr/include
        /usr/local/include
    PATH_SUFFIXES
        onnxruntime/core/session
        onnxruntime
)

find_library(ONNXRUNTIME_LIBRARY
    NAMES onnxruntime
    HINTS
        ${ONNXRUNTIME_ROOT}/lib
        /usr/lib
        /usr/local/lib
        /usr/lib/x86_64-linux-gnu
        /usr/lib/aarch64-linux-gnu
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ONNXRuntime
    REQUIRED_VARS ONNXRUNTIME_LIBRARY ONNXRUNTIME_INCLUDE_DIR
)

if(ONNXRuntime_FOUND AND NOT TARGET ONNXRuntime::ONNXRuntime)
    add_library(ONNXRuntime::ONNXRuntime UNKNOWN IMPORTED)
    set_target_properties(ONNXRuntime::ONNXRuntime PROPERTIES
        IMPORTED_LOCATION "${ONNXRUNTIME_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ONNXRUNTIME_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(ONNXRUNTIME_INCLUDE_DIR ONNXRUNTIME_LIBRARY)
