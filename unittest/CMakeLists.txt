include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com.cnpmjs.org/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_subdirectory(Utils)
add_subdirectory(Diagnostic)
add_subdirectory(Lex)
add_subdirectory(Parser)
add_subdirectory(Sema)

enable_testing()

file(GLOB_RECURSE all_test_source_files
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
LIST(APPEND all_test_used_libraries gtest_main utils diag lex parse sema)
add_executable(AllTest ${all_test_source_files})
target_link_libraries(AllTest PRIVATE ${all_test_used_libraries})
target_include_directories(AllTest PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/unittest/include
        )
include(GoogleTest)
gtest_discover_tests(AllTest)
