project(IrisAnalysisLib)
cmake_minimum_required(VERSION 3.3)

file(GLOB IRIS_ANALYSIS_SRC *.h *.hpp *.cpp)
set(IRISLIB_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../Iris)

include_directories(
   ${OpenCV_INCLUDE_DIRS}
)

link_directories(
   ${OpenCV_LIBRARY_DIRS}
)
message(${CMAKE_CURRENT_SOURCE_DIR}../Iris)
include_directories(${OPENCV_INCLUDE_DIR}
                    ${CMAKE_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}../Iris
                    ${IRISLIB_SRC_DIR})

add_library(IrisAnalysisLib STATIC ${IRIS_ANALYSIS_SRC})
target_link_libraries(IrisAnalysisLib ${OpenCV_LIBS} /usr/local/eyedentiscan/lib/IrisLib)
add_dependencies(IrisAnalysisLib IrisLib)

# Install libs and header files
install(TARGETS IrisAnalysisLib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION include)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
        FILES_MATCHING PATTERN "*h")
