add_executable(DiagTest DiagTest.cpp)
target_link_libraries(DiagTest PRIVATE gtest_main diag)
target_include_directories(DiagTest
        PRIVATE
        ${CMAKE_SOURCE_DIR}/include)