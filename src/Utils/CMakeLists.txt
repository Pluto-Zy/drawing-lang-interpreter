list(APPEND _source_files "StringRef.cpp")
add_library(utils ${_source_files})
target_include_directories(utils PRIVATE ${CMAKE_SOURCE_DIR}/include)