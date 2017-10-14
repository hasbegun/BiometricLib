set(FaceLib FaceLib_Project)

file(GLOB FACE_SRC *.h *.hpp *.cpp)

include_directories(
   ${OpenCV_INCLUDE_DIRS}
)

link_directories(
   ${OpenCV_LIBRARY_DIRS}
)

include_directories(${OPENCV_INCLUDE_DIR}  ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

add_library(FaceLib STATIC ${FACE_SRC})
target_link_libraries(FaceLib ${OpenCV_LIBS})

# Install libs and header files
install(TARGETS FaceLib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION include)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include
        FILES_MATCHING PATTERN "*h")
