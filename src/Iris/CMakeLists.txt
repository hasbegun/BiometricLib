project(IrisLib)
cmake_minimum_required(VERSION 3.3)

file(GLOB IRIS_SRC Iris/*.h Iris/*.hpp Iris/*.cpp)
file(GLOB MASEK_SRC Masek/*.h Masek/*.hpp Masek/*.cpp)

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
