add_executable(UtilsTest StringRefTest.cpp)
target_link_libraries(UtilsTest PRIVATE gtest_main utils)
target_include_directories(UtilsTest PRIVATE ${CMAKE_SOURCE_DIR}/include)