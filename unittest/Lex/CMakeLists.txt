add_executable(LexTest LexTest.cpp)
target_link_libraries(LexTest PRIVATE gtest_main lex)
target_include_directories(LexTest PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/unittest/include
        )