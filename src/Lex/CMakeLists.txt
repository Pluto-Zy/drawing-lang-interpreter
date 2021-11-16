list(APPEND _source_files "Lexer.cpp")
add_library(lex ${_source_files})
target_include_directories(lex PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(lex PRIVATE utils diag)
