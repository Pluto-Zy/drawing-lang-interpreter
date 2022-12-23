list(APPEND _source_files "Sema.cpp" "IdentifierInfo.cpp" "SemaExpr.cpp" "SemaType.cpp")
add_library(sema ${_source_files})
target_include_directories(sema PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(sema PRIVATE utils diag)