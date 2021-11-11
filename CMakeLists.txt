cmake_minimum_required(VERSION 3.5)
project(drawing-lang-interpreter LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_subdirectory(src)
add_subdirectory("unittest")
