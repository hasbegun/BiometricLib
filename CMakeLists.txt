project(BiometricLib)
cmake_minimum_required(VERSION 3.3)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_definitions(-DDEBUG -g -O3 -Wall -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE)

# find opencv pkg with specific version and path.
# otherwise it will find the pkg at the PATH usuallo /usr/local/lib
find_package(OpenCV 2.4.13.3 REQUIRED PATHS /usr/local/opencv/2.4.13.3)
message( ${OpenCV_VERSION} )
message("-- OPENCV include: " ${OpenCV_INCLUDE_DIRS})

if (${OpenCV_VERSION} VERSION_LESS 2.4.9)
    message(FATAL_ERROR "OpenCV version is not compatible : ${OpenCV_VERSION}")
endif()

include_directories(
   ${OpenCV_INCLUDE_DIRS}
)

link_directories(
   ${OpenCV_LIBRARY_DIRS}
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_DIR}/build)
set(CMAKE_BUILD_DIRECTORY ${CMAKE_CURRENT_DIR}/build)
set(CMAKE_BUILD_FILES_DIRECTORY ${CMAKE_CURRENT_DIR}/build)
set(CMAKE_BINARY_DIR ${CMAKE_CURRENT_DIR}/bin)

file(GLOB IRIS_SRC src/Iris/*.h src/Iris/*.hpp src/Iris/*.cpp)
file(GLOB MASEK_SRC src/Masek/*.h src/Masek/*.hpp src/Masek/*.cpp)
file(GLOB FACE_SRC src/Face/*.h src/Face/*.hpp src/Face/*.cpp)
file(GLOB ANALYSIS_SRC src/IrisAnalysis/*.h src/IrisAnalysis/*.hpp src/IrisAnalysis/*.cpp)

include_directories(${OPENCV_INCLUDE_DIR}  ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})

add_library(IrisLib STATIC ${IRIS_SRC} ${MASEK_SRC})
target_link_libraries(IrisLib ${OpenCV_LIBS})

add_library(IrisAnalysisLib STATIC ${ANALYSIS_SRC})
target_link_libraries(IrisAnalysisLib ${OpenCV_LIBS})

add_library(FaceLib STATIC ${FACE_SRC})
target_link_libraries(FaceLib ${OpenCV_LIBS})

# Install libs and header files 
install(TARGETS IrisLib IrisAnalysisLib FaceLib 
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION include)
install(DIRECTORY src/ DESTINATION include
        FILES_MATCHING PATTERN "*h")
        
