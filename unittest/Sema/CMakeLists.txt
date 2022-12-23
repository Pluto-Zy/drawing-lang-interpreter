add_executable(SemaTest IdentifierInfoTest.cpp EvaluateTest.cpp)
target_link_libraries(SemaTest PRIVATE gtest_main sema parse internal interpret)
target_include_directories(SemaTest PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/unittest/include
        )
