list(APPEND _source_files "Interpreter.cpp")
add_library(interpret ${_source_files})
target_include_directories(interpret PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_subdirectory(InternalSupport)
target_link_libraries(interpret PUBLIC parse sema internal)

add_executable(drawing Driver.cpp)
target_include_directories(drawing PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(drawing PUBLIC utils lex parse ast sema ${OpenCV_LIBS} internal interpret)
