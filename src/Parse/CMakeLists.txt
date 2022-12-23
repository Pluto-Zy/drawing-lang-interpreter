list(APPEND _source_files "Parser.cpp")
add_library(parse ${_source_files})
target_include_directories(parse PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(parse PRIVATE utils diag lex ast)