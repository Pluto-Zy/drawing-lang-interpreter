list(APPEND _source_files "Stmt.cpp" "Expr.cpp")
add_library(ast ${_source_files})
target_include_directories(ast PRIVATE ${CMAKE_SOURCE_DIR}/include)