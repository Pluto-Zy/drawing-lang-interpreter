add_executable(ParseTest ParseExprTest.cpp ParseStmtTest.cpp)
target_link_libraries(ParseTest PUBLIC gtest_main parse)
target_include_directories(ParseTest
        PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/unittest/include
        )