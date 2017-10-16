project(BiometricLib)
cmake_minimum_required(VERSION 3.3)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_definitions(-DDEBUG -g -O3 -Wall -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_DIR}/build)
set(CMAKE_BUILD_DIRECTORY ${CMAKE_CURRENT_DIR}/build)
set(CMAKE_BUILD_FILES_DIRECTORY ${CMAKE_CURRENT_DIR}/build)
set(CMAKE_BINARY_DIR ${CMAKE_CURRENT_DIR}/bin)

add_subdirectory(src/Face)
add_subdirectory(src/Iris)
add_subdirectory(src/IrisAnalysis)
