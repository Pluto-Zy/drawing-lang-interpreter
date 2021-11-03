set(gtest_force_shared_crt ON CACHE BOOL "Always use msvcrt.dll" FORCE)
add_subdirectory(googletest)
add_subdirectory(Utils)
enable_testing()
