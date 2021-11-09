list(APPEND _source_files "DiagBuilder.cpp" "DiagEngine.cpp" "DiagConsumer.cpp")
add_library(diag ${_source_files})
target_include_directories(diag PRIVATE ${CMAKE_SOURCE_DIR}/include)