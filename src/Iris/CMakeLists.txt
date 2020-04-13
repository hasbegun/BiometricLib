project(IrisLib)
cmake_minimum_required(VERSION 3.3)

file(GLOB IRIS_SRC Iris/*.h Iris/*.hpp Iris/*.cpp)
file(GLOB MASEK_SRC Masek/*.h Masek/*.hpp Masek/*.cpp)

# find opencv pkg with specific version and path.
# otherwise it will find the pkg at the PATH /usr/local/lib
# find_package(OpenCV 2.4.13.6 REQUIRED PATHS /usr/local/opencv/2.4.13.6)
if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    message("-- System: Mac OS")
    find_package(OpenCV 3.4 REQUIRED PATHS /usr/local/opt/opencv@3)

elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    message("-- System: Linux")
    find_package(OpenCV 3.4 REQUIRED)
endif()

message("-- OCV version: " ${OpenCV_VERSION} )
message("-- OCV include: " ${OpenCV_INCLUDE_DIRS})
message("-- OCV libs: " ${OpenCV_LIBS})

if (${OpenCV_VERSION} VERSION_LESS 3.4)
    message(FATAL_ERROR "OpenCV version is not compatible : ${OpenCV_VERSION}")
endif()

include_directories(${CMAKE_SOURCE_DIR}/Iris
                    ${CMAKE_SOURCE_DIR}/Masek
                    ${CMAKE_CURRENT_SOURCE_DIR})

add_library(${PROJECT_NAME} STATIC ${IRIS_SRC} ${MASEK_SRC})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

# Install libs and header files
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION include)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Iris DESTINATION include
        FILES_MATCHING PATTERN "*h")
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Masek DESTINATION include
        FILES_MATCHING PATTERN "*h")
