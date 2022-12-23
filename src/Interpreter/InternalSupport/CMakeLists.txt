set(BUILD_SHARED_LIBS OFF)
set(OpenCV_STATIC ON)
find_package(OpenCV REQUIRED)
add_library(internal InternalImpl.cpp InternalFuncImpl.cpp)
target_include_directories(internal PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS})
target_link_libraries(internal PUBLIC ${OpenCV_LIBS})
