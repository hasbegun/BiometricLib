project(IrisLib)
cmake_minimum_required(VERSION 3.3)

file(GLOB IRIS_SRC Iris/*.h Iris/*.hpp Iris/*.cpp)
file(GLOB MASEK_SRC Masek/*.h Masek/*.hpp Masek/*.cpp)

# find opencv pkg with specific version and path.
# otherwise it will find the pkg at the PATH usuallo /usr/local/lib
find_package(OpenCV 2.4.13.6 REQUIRED PATHS /usr/local/opencv/2.4.13.6)
message( ${OpenCV_VERSION} )
message("-- OPENCV include: " ${OpenCV_INCLUDE_DIRS})

if (${OpenCV_VERSION} VERSION_LESS 2.4.13.6)
    message(FATAL_ERROR "OpenCV version is not compatible : ${OpenCV_VERSION}")
endif()

include_directories(
   ${OpenCV_INCLUDE_DIRS}
)

link_directories(
   ${OpenCV_LIBRARY_DIRS}
)

include_directories(${OPENCV_INCLUDE_DIR}
                    ${CMAKE_SOURCE_DIR}/Iris
                    ${CMAKE_SOURCE_DIR}/Masek
                    ${CMAKE_CURRENT_SOURCE_DIR})

add_library(IrisLib STATIC ${IRIS_SRC} ${MASEK_SRC})
target_link_libraries(IrisLib ${OpenCV_LIBS})

# Install libs and header files
install(TARGETS IrisLib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION include)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Iris DESTINATION include
        FILES_MATCHING PATTERN "*h")
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Masek DESTINATION include
        FILES_MATCHING PATTERN "*h")
