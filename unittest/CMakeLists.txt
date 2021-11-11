set(gtest_force_shared_crt ON CACHE BOOL "Always use msvcrt.dll" FORCE)
add_subdirectory(googletest)
add_subdirectory(Utils)
add_subdirectory(Diagnostic)

file(GLOB_RECURSE all_test_source_files
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
LIST(APPEND all_test_used_libraries gtest_main utils diag)
add_executable(AllTest ${all_test_source_files})
target_link_libraries(AllTest PRIVATE ${all_test_used_libraries})
target_include_directories(AllTest PRIVATE ${CMAKE_SOURCE_DIR}/include)
enable_testing()
