project(FaceLib)
cmake_minimum_required(VERSION 3.3)

file(GLOB FACE_SRC *.h *.hpp *.cpp)

# find opencv pkg with specific version and path.
# otherwise it will find the pkg at the PATH usuallo /usr/local/lib
find_package(OpenCV 3 REQUIRED PATH /usr/local/opencv3)
message( ${OpenCV_VERSION} )
message("-- OPENCV include: " ${OpenCV_INCLUDE_DIRS})

if (${OpenCV_VERSION} VERSION_LESS 3)
    message(FATAL_ERROR "OpenCV version is not compatible : ${OpenCV_VERSION}")
endif()

# include_directories(
#    /usr/local/include
# )

# link_directories(
#    /usr/local/lib
# )

# include_directories(
#    ${OpenCV_INCLUDE_DIRS}
# )

# link_directories(
#    ${OpenCV_LIBRARY_DIRS}
# )

# include_directories(${OPENCV_INCLUDE_DIR} ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

add_library(FaceLib STATIC ${FACE_SRC})
# target_link_libraries(FaceLib ${OpenCV_LIBS})

# Install libs and header files
install(TARGETS FaceLib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION include)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
        FILES_MATCHING PATTERN "*h")
